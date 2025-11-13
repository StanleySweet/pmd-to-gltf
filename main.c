#include "pmd_psa_types.h"
#include "skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Function declarations from other modules
PMDModel* load_pmd(const char *filename);
void free_pmd(PMDModel *model);
PSAAnimation* load_psa(const char *filename);
void free_psa(PSAAnimation *anim);
void export_gltf(const char *output_file, PMDModel *model, PSAAnimation **anims, uint32_t anim_count, SkeletonDef *skel, const char *mesh_name);

// Auto-detect skeleton XML file based on PMD filename
static char* find_skeleton_file(const char *pmd_file) {
    // Replace .pmd with .xml
    char *skel_file = malloc(strlen(pmd_file) + 10);
    strcpy(skel_file, pmd_file);

    char *ext = strrchr(skel_file, '.');
    if (ext && strcmp(ext, ".pmd") == 0) {
        strcpy(ext, ".xml");

        FILE *f = fopen(skel_file, "r");
        if (f) {
            fclose(f);
            return skel_file;
        }
    }

    free(skel_file);
    return NULL;
}

// Extract animation name from PSA filename
// Pattern: basename_animname.psa -> "animname"
// For horse_idle_a.psa with basename "horse" -> "idle_a"
static char* extract_anim_name(const char *psa_file, const char *basename) {
    // Find last path separator
    const char *filename = strrchr(psa_file, '/');
    if (!filename) filename = strrchr(psa_file, '\\');
    filename = filename ? filename + 1 : psa_file;

    // Find the extension
    const char *ext = strrchr(filename, '.');
    if (!ext) return NULL;

    // Build the prefix to skip: "basename_"
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "%s_", basename);
    size_t prefix_len = strlen(prefix);

    // Check if filename starts with prefix
    if (strncmp(filename, prefix, prefix_len) == 0) {
        // Extract everything between prefix and extension
        const char *start = filename + prefix_len;
        size_t len = ext - start;
        char *name = malloc(len + 1);
        memcpy(name, start, len);
        name[len] = '\0';
        return name;
    }

    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s <base_name> [output.gltf] [skeleton_id] [--print-bones]\n", argv[0]);
        printf("  Loads: <base_name>.pmd, <base_name>.xml, <base_name>_*.psa\n");
        printf("  Example: %s horse output.gltf Horse\n", argv[0]);
        printf("  Option: --print-bones to print all bone transforms and exit.\n");
        return 1;
    }

    const char *base_name = argv[1];
    const char *output_file = (argc >= 3) ? argv[2] : "output.gltf";
    const char *skeleton_id = (argc >= 4) ? argv[3] : "Armature";
    int print_bones = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--print-bones") == 0) print_bones = 1;
    }

    // Build filenames from base name
    char pmd_file[512];
    char skeleton_file[512];
    snprintf(pmd_file, sizeof(pmd_file), "%s.pmd", base_name);
    snprintf(skeleton_file, sizeof(skeleton_file), "%s.xml", base_name);


    printf("Loading PMD: %s\n", pmd_file);
    PMDModel *model = load_pmd(pmd_file);
    if (!model) {
        fprintf(stderr, "Failed to load PMD file\n");
        return 1;
    }

    printf("  Vertices: %u, Faces: %u, Bones: %u\n",
           model->numVertices, model->numFaces, model->numBones);

    if (model->numPropPoints > 0) {
        printf("  Prop points: %u\n", model->numPropPoints);
    }

    if (print_bones) {
        printf("All bone transforms (rest pose):\n");
        for (uint32_t i = 0; i < model->numBones; i++) {
            printf("Bone %2u: T(% .2f,% .2f,% .2f) R(% .2f,% .2f,% .2f,% .2f)\n",
                   i,
                   model->restStates[i].translation.x,
                   model->restStates[i].translation.y,
                   model->restStates[i].translation.z,
                   model->restStates[i].rotation.x,
                   model->restStates[i].rotation.y,
                   model->restStates[i].rotation.z,
                   model->restStates[i].rotation.w);
        }
        free_pmd(model);
        return 0;
    }

    // Load skeleton hierarchy
    printf("Loading skeleton: %s (id: %s)\n", skeleton_file, skeleton_id);
    SkeletonDef *skel = load_skeleton_xml(skeleton_file, skeleton_id);
    if (skel) {
        printf("  Loaded %d bones\n", skel->bone_count);
        if (model->numBones > (uint32_t)skel->bone_count) {
            printf("  Note: %u extra bones\n", model->numBones - skel->bone_count);
        }
    }

    // Find and load all matching PSA files
    printf("Loading animations: %s_*.psa\n", base_name);
    PSAAnimation **anims = NULL;
    uint32_t anim_count = 0;
    uint32_t anim_capacity = 10;
    anims = calloc(anim_capacity, sizeof(PSAAnimation*));

    char psa_pattern[512];
    snprintf(psa_pattern, sizeof(psa_pattern), "%s_", base_name);
    size_t pattern_len = strlen(psa_pattern);

    // Get directory from base_name
    const char *dir_end = strrchr(base_name, '/');
    if (!dir_end) dir_end = strrchr(base_name, '\\');
    char dir[512] = ".";
    if (dir_end) {
        size_t dir_len = dir_end - base_name;
        if (dir_len < sizeof(dir) - 1) {
            memcpy(dir, base_name, dir_len);
            dir[dir_len] = '\0';
        }
    }

    // Load all matching PSA files (Windows-specific directory reading)
    #ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "%s_*.psa", base_name);
    HANDLE hFind = FindFirstFileA(search_pattern, &find_data);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            char psa_file[512];
            const char *dir_part = dir_end ? "" : "";
            if (dir_end) {
                snprintf(psa_file, sizeof(psa_file), "%s/%s", dir, find_data.cFileName);
            } else {
                snprintf(psa_file, sizeof(psa_file), "%s", find_data.cFileName);
            }

            PSAAnimation *anim = load_psa(psa_file);
            if (anim) {
                // Extract just the base filename for animation naming
                const char *base_filename = strrchr(base_name, '/');
                if (!base_filename) base_filename = strrchr(base_name, '\\');
                base_filename = base_filename ? base_filename + 1 : base_name;

                char *anim_name = extract_anim_name(psa_file, base_filename);
                if (anim_name) {
                    free(anim->name);
                    anim->name = anim_name;
                }

                if (anim_count >= anim_capacity) {
                    anim_capacity *= 2;
                    anims = realloc(anims, anim_capacity * sizeof(PSAAnimation*));
                }
                anims[anim_count++] = anim;
                printf("  Loaded: %s (%u frames)\n", anim->name, anim->numFrames);
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
    #else
    // Fallback: try common animation names
    const char *anim_names[] = {"idle", "walk", "run", "attack", "death"};
    for (int i = 0; i < 5; i++) {
        char psa_file[512];
        snprintf(psa_file, sizeof(psa_file), "%s_%s.psa", base_name, anim_names[i]);
        PSAAnimation *anim = load_psa(psa_file);
        if (anim) {
            free(anim->name);
            anim->name = strdup(anim_names[i]);
            if (anim_count >= anim_capacity) {
                anim_capacity *= 2;
                anims = realloc(anims, anim_capacity * sizeof(PSAAnimation*));
            }
            anims[anim_count++] = anim;
            printf("  Loaded: %s (%u frames)\n", anim->name, anim->numFrames);
        }
    }
    #endif

    if (anim_count == 0) {
        fprintf(stderr, "Warning: No animations found\n");
    }

    printf("Exporting to glTF: %s\n", output_file);

    // Extract mesh name from base_name
    const char *mesh_name = strrchr(base_name, '/');
    if (!mesh_name) mesh_name = strrchr(base_name, '\\');
    mesh_name = mesh_name ? mesh_name + 1 : base_name;

    export_gltf(output_file, model, anims, anim_count, skel, mesh_name);

    printf("Done! Exported %u animation(s)\n", anim_count);

    if (skel) free_skeleton(skel);
    for (uint32_t i = 0; i < anim_count; i++) {
        free_psa(anims[i]);
    }
    free(anims);
    free_pmd(model);
    return 0;
}

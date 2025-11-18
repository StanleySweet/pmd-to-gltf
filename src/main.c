#include "pmd_psa_types.h"
#include "skeleton.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

// Function declarations from other modules
PMDModel* load_pmd(const char *filename);
void free_pmd(PMDModel *model);
PSAAnimation* load_psa(const char *filename);
void free_psa(PSAAnimation *anim);
int export_gltf(const char *output_file, PMDModel *model, PSAAnimation **anims, uint32_t anim_count, SkeletonDef *skel, const char *mesh_name, const float *anim_speed_percent, const char *rest_pose_anim);
char* get_first_skeleton_id(const char *filename);



// Extract animation name from PSA filename
// Pattern: basename_animname.psa -> "animname"
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
        printf("Usage: %s <base_name> [--print-bones]\n", argv[0]);
        printf("  Loads: <base_name>.pmd, <base_name>.json, <base_name>_*.psa\n");
        printf("  Outputs: output/<filename>.gltf\n");
        printf("  Example: %s input/model\n", argv[0]);
        printf("  Option: --print-bones to print all bone transforms and exit.\n");
        return 1;
    }

    const char *base_name = argv[1];

    // Option flags
    int print_bones = 0;
    const char *rest_pose_anim = NULL;
    // Only positional args before any --option are used for skeleton detection
    int first_option = 2;
    for (int i = 2; i < argc; ++i) {
        if (argv[i][0] == '-') { first_option = i; break; }
    }
    for (int i = first_option; i < argc; ++i) {
        if (strcmp(argv[i], "--print-bones") == 0) print_bones = 1;
        if (strcmp(argv[i], "--rest-pose") == 0 && i+1 < argc) {
            rest_pose_anim = argv[i+1];
            i++;
        }
    }

    // Utilisation du JSON pour squelette et vitesses anims
    char pmd_file[512];
    char skeleton_json_file[512];
    char output_file[512];
    snprintf(pmd_file, sizeof(pmd_file), "%s.pmd", base_name);
    snprintf(skeleton_json_file, sizeof(skeleton_json_file), "%s.json", base_name);

    const char *output_basename = strrchr(base_name, '/');
    if (!output_basename) output_basename = strrchr(base_name, '\\');
    output_basename = output_basename ? output_basename + 1 : base_name;
    snprintf(output_file, sizeof(output_file), "output/%s.gltf", output_basename);


    printf("Loading PMD: %s\n", pmd_file);
    PMDModel *model = load_pmd(pmd_file);
    if (!model) {
        fprintf(stderr, "Failed to load PMD file\n");
        return 1;
    }

    printf("  PMD v%u: Vertices=%u, Faces=%u, Bones=%u, Props=%u\n",
           model->version, model->numVertices, model->numFaces, model->numBones, model->numPropPoints);

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

    // Charger le squelette depuis le JSON
    SkeletonDef *skel = load_skeleton_json(skeleton_json_file);
    if (skel) {
        printf("Skeleton: %s\n", skel->title);
        printf("  Loaded %d bones\n", skel->bone_count);
        if (model->numBones > (uint32_t)skel->bone_count) {
            printf("  Note: %u extra bones\n", model->numBones - skel->bone_count);
        }
    }

    // Find and load all matching PSA files
    PSAAnimation **anims = NULL;
    uint32_t anim_count = 0;
    uint32_t anim_capacity = 10;
    anims = calloc(anim_capacity, sizeof(PSAAnimation*));

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

    snprintf(skeleton_json_file, sizeof(skeleton_json_file), "%s.json", base_name);

    // Extract just the base filename for pattern matching
    const char *base_filename = dir_end ? dir_end + 1 : base_name;
    
    printf("Loading animations: %s_*.psa\n", base_filename);

    // Create pattern for PSA files
    char psa_pattern[256];
    snprintf(psa_pattern, sizeof(psa_pattern), "%s_*.psa", base_filename);

    // Find all matching PSA files
    FileList *psa_files = find_files(dir, psa_pattern);

    if (psa_files && psa_files->count > 0) {
        for (uint32_t i = 0; i < psa_files->count; i++) {
            PSAAnimation *anim = load_psa(psa_files->paths[i]);
            if (anim) {
                char *anim_name = extract_anim_name(psa_files->paths[i], base_filename);
                if (anim_name) {
                    free(anim->name);
                    anim->name = anim_name;
                } else {
                    // If we couldn't extract a name, warn about legacy "God Knows"
                    if (anim->name && strcmp(anim->name, "God Knows") == 0) {
                        fprintf(stderr, "Warning: Animation file '%s' has legacy 'God Knows' placeholder name.\n", psa_files->paths[i]);
                    }
                }
                if (anim_count >= anim_capacity) {
                    anim_capacity *= 2;
                    anims = realloc(anims, anim_capacity * sizeof(PSAAnimation*));
                }
                anims[anim_count++] = anim;
            }
        }
    }

    if (psa_files) {
        free_file_list(psa_files);
    }

    if (anim_count == 0) {
        fprintf(stderr, "Warning: No animations found\n");
    }

    // Charger les vitesses d'animation depuis le JSON
    float *anim_speeds = NULL;
    if (anim_count > 0) {
        anim_speeds = calloc(anim_count, sizeof(float));
        FILE *f = fopen(skeleton_json_file, "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *content = malloc((size_t)size + 1);
            fread(content, 1, (size_t)size, f);
            content[size] = '\0';
            fclose(f);
            cJSON *root = cJSON_Parse(content);
            free(content);
            if (root) {
                cJSON *speeds = cJSON_GetObjectItem(root, "animation_speeds");
                for (uint32_t i = 0; i < anim_count; i++) {
                    float speed = 100.0f;
                    if (anims[i]->name && speeds) {
                        cJSON *val = cJSON_GetObjectItem(speeds, anims[i]->name);
                        if (val && cJSON_IsNumber(val)) {
                            speed = (float)val->valuedouble;
                        }
                    }
                    anim_speeds[i] = speed;
                    printf("  %s: PSA v1 (%u bones, %u frames) @ %.1f%%",
                           anims[i]->name, anims[i]->numBones, anims[i]->numFrames, anim_speeds[i]);
                    #ifdef PSA_HAS_PROPPOINTS
                    printf(" | PropPoints=%u", anims[i]->numPropPoints);
                    #endif
                    printf("\n");
                }
                cJSON_Delete(root);
            }
        }
    }

    printf("Exporting to glTF: %s\n", output_file);

    // Extract mesh name from base_name
    const char *mesh_name = strrchr(base_name, '/');
    if (!mesh_name) mesh_name = strrchr(base_name, '\\');
    mesh_name = mesh_name ? mesh_name + 1 : base_name;

    int export_status = export_gltf(output_file, model, anims, anim_count, skel, mesh_name, anim_speeds, rest_pose_anim);
    if (!export_status) {
        fprintf(stderr, "Error: Export failed\n");
        free(anim_speeds);
        if (skel) free_skeleton(skel);
        for (uint32_t i = 0; i < anim_count; i++) {
            free_psa(anims[i]);
        }
        free(anims);
        free_pmd(model);
        return 1;
    }

    printf("Done! Exported %u animation(s)\n", anim_count);

    free(anim_speeds);

    // Cleanup
    if (skel) free_skeleton(skel);
    for (uint32_t i = 0; i < anim_count; i++) {
        free_psa(anims[i]);
    }
    free(anims);
    free_pmd(model);
    return 0;
}

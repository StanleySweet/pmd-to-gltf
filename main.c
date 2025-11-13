#include "pmd_psa_types.h"
#include "skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function declarations from other modules
PMDModel* load_pmd(const char *filename);
void free_pmd(PMDModel *model);
PSAAnimation* load_psa(const char *filename);
void free_psa(PSAAnimation *anim);
void export_gltf(const char *output_file, PMDModel *model, PSAAnimation *anim, SkeletonDef *skel, const char *mesh_name);

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
// Pattern: model_animname.psa -> "animname"
static char* extract_anim_name(const char *psa_file) {
    // Find last path separator
    const char *filename = strrchr(psa_file, '/');
    if (!filename) filename = strrchr(psa_file, '\\');
    filename = filename ? filename + 1 : psa_file;

    // Find last underscore before extension
    const char *ext = strrchr(filename, '.');
    const char *underscore = strrchr(filename, '_');

    if (underscore && ext && underscore < ext) {
        // Extract between underscore and extension
        size_t len = ext - underscore - 1;
        char *name = malloc(len + 1);
        memcpy(name, underscore + 1, len);
        name[len] = '\0';
        return name;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <model.pmd> <animation.psa> [output.gltf] [skeleton.xml] [skeleton_id]\n", argv[0]);
        printf("  If skeleton.xml is not specified, will auto-detect <model>.xml\n");
        return 1;
    }

    const char *pmd_file = argv[1];
    const char *psa_file = argv[2];
    const char *output_file = (argc >= 4) ? argv[3] : "output.gltf";
    const char *skeleton_file = (argc >= 5) ? argv[4] : NULL;
    const char *skeleton_id = (argc >= 6) ? argv[5] : "Horse";

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

    printf("Loading PSA: %s\n", psa_file);
    PSAAnimation *anim = load_psa(psa_file);
    if (!anim) {
        fprintf(stderr, "Failed to load PSA file\n");
        free_pmd(model);
        return 1;
    }

    // Override animation name from filename if possible
    char *anim_name_from_file = extract_anim_name(psa_file);
    if (anim_name_from_file) {
        free(anim->name);
        anim->name = anim_name_from_file;
    }

    printf("  Animation: %s, Frames: %u, Bones: %u\n",
           anim->name, anim->numFrames, anim->numBones);

    if (model->numBones != anim->numBones) {
        fprintf(stderr, "Warning: Bone count mismatch (PMD: %u, PSA: %u)\n",
                model->numBones, anim->numBones);
    }

    // Load skeleton hierarchy
    SkeletonDef *skel = NULL;
    char *auto_skel_file = NULL;

    if (!skeleton_file) {
        auto_skel_file = find_skeleton_file(pmd_file);
        skeleton_file = auto_skel_file;
    }

    if (skeleton_file) {
        printf("Loading skeleton: %s (id: %s)\n", skeleton_file, skeleton_id);
        skel = load_skeleton_xml(skeleton_file, skeleton_id);
        if (skel) {
            printf("  Loaded %d bones\n", skel->bone_count);

            // Extra bones beyond skeleton are likely prop points
            if (model->numBones > (uint32_t)skel->bone_count) {
                printf("  Note: %u extra bones (likely prop points)\n",
                       model->numBones - skel->bone_count);
            }
        }
    }

    printf("Exporting to glTF: %s\n", output_file);

    // Extract mesh name from PMD filename
    const char *mesh_name = strrchr(pmd_file, '/');
    if (!mesh_name) mesh_name = strrchr(pmd_file, '\\');
    mesh_name = mesh_name ? mesh_name + 1 : pmd_file;

    // Remove extension
    char mesh_name_buf[256];
    strncpy(mesh_name_buf, mesh_name, sizeof(mesh_name_buf) - 1);
    mesh_name_buf[sizeof(mesh_name_buf) - 1] = '\0';
    char *ext = strrchr(mesh_name_buf, '.');
    if (ext) *ext = '\0';

    export_gltf(output_file, model, anim, skel, mesh_name_buf);

    printf("Done!\n");

    if (auto_skel_file) free(auto_skel_file);
    if (skel) free_skeleton(skel);
    free_psa(anim);
    free_pmd(model);
    return 0;
}

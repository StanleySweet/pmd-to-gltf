#ifndef PMD_WRITER_H
#define PMD_WRITER_H

#include "pmd_psa_types.h"
#include <stdio.h>

// Write PMD file to disk
int write_pmd(const char *filename, const PMDModel *model);

// Write PSA file to disk
int write_psa(const char *filename, const PSAAnimation *anim);

#include "../src/skeleton.h"
int export_gltf(const char *output_file, PMDModel *model, PSAAnimation **anims, uint32_t anim_count, SkeletonDef *skel, const char *mesh_name, const float *anim_speed_percent, const char *rest_pose_anim);
#endif

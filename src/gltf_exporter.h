#ifndef GLTF_EXPORTER_H
#define GLTF_EXPORTER_H

#include "pmd_psa_types.h"
#include "skeleton.h"

#ifdef __cplusplus
extern "C" {
#endif

int export_gltf(const char *output_file, PMDModel *model, PSAAnimation **anims, uint32_t anim_count, SkeletonDef *skel, const char *mesh_name, const float *anim_speed_percent, const char *rest_pose_anim);

#ifdef __cplusplus
}
#endif

#endif // GLTF_EXPORTER_H

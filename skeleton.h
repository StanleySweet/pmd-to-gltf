#ifndef SKELETON_H
#define SKELETON_H

#include <stdint.h>

#define MAX_BONES 64
#define MAX_BONE_NAME 64

typedef struct {
    char name[MAX_BONE_NAME];
    int parent_index;  // -1 for root
} BoneInfo;

typedef struct {
    int bone_count;
    BoneInfo bones[MAX_BONES];
} SkeletonDef;

// Parse skeleton XML and return bone hierarchy
SkeletonDef* load_skeleton_xml(const char *filename, const char *skeleton_id);
void free_skeleton(SkeletonDef *skel);

#endif

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
    char skeleton_file[256];
    char skeleton_id[64];
    char title[128];
} SkeletonDef;

// Parse skeleton JSON and return bone hierarchy
SkeletonDef* load_skeleton_json(const char *filename);
// Extract the first skeleton ID from XML file
char* get_first_skeleton_id(const char *filename);
void free_skeleton(SkeletonDef *skel);

#endif

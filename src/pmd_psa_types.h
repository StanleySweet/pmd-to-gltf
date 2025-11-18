#ifndef PMD_PSA_TYPES_H
#define PMD_PSA_TYPES_H

#include <stdint.h>

// Basic types matching the file format specs
typedef struct {
    float x, y, z;
} Vector3D;

typedef struct {
    float x, y, z, w;
} Quaternion;

typedef struct {
    float u, v;
} TexCoord;

typedef struct {
    uint8_t bones[4];    // bone indices, 0xFF = no bone
    float weights[4];     // sum to 1.0
} VertexBlend;

typedef struct {
    Vector3D position;
    Vector3D normal;
    TexCoord *coords;     // array of size numTexCoords
    VertexBlend blend;
} Vertex;

typedef struct {
    uint16_t vertices[3];
} Face;

typedef struct {
    Vector3D translation;
    Quaternion rotation;
} BoneState;

typedef struct {
    char *name;
    Vector3D translation;
    Quaternion rotation;
    uint8_t bone;
} PropPoint;

// PMD model structure
typedef struct {
    uint32_t version;
    uint32_t numVertices;
    uint32_t numTexCoords;
    Vertex *vertices;
    uint32_t numFaces;
    Face *faces;
    uint32_t numBones;
    BoneState *restStates;
    uint32_t numPropPoints;
    PropPoint *propPoints;
} PMDModel;

// PSA animation structure
typedef struct {
    char *name;
    float frameLength;
    uint32_t numBones;
    uint32_t numFrames;
    BoneState *boneStates;  // size: numBones * numFrames
} PSAAnimation;

// Function declarations
PMDModel* load_pmd(const char *filename);
void free_pmd(PMDModel *model);
PSAAnimation* load_psa(const char *filename);
void free_psa(PSAAnimation *anim);

#endif

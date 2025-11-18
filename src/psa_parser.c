#include "pmd_psa_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Reuse read helpers (these should be in a shared header in production)
static uint32_t read_u32(FILE *f) {
    uint8_t buf[4];
    if (fread(buf, 1, 4, f) != 4) return 0;
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static float read_float(FILE *f) {
    float val;
    if (fread(&val, sizeof(float), 1, f) != 1) return 0.0f;
    return val;
}

static Vector3D read_vec3(FILE *f) {
    Vector3D v;
    v.x = read_float(f);
    v.y = read_float(f);
    v.z = read_float(f);
    return v;
}

static Quaternion read_quat(FILE *f) {
    Quaternion q;
    q.x = read_float(f);
    q.y = read_float(f);
    q.z = read_float(f);
    q.w = read_float(f);
    return q;
}

// Load PSA file
PSAAnimation* load_psa(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return NULL;
    }

    PSAAnimation *anim = calloc(1, sizeof(PSAAnimation));
    uint32_t version = 1; // default for error handling

    // Read header
    char magic[4];
    if (fread(magic, 1, 4, f) != 4) {
        fprintf(stderr, "Failed to read PSA magic\n");
        free(anim);
        fclose(f);
        return NULL;
    }
    if (memcmp(magic, "PSSA", 4) != 0) {
        fprintf(stderr, "Invalid PSA magic\n");
        free(anim);
        fclose(f);
        return NULL;
    }

    // Read and validate version
    version = read_u32(f);
    if (version != 1) {
        fprintf(stderr, "Warning: Unsupported PSA version %u (expected 1)\n", version);
    }
    
    // Skip data size (not used)
    read_u32(f); // data_size

    // Read name
    uint32_t nameLen = read_u32(f);
    anim->name = calloc(nameLen + 1, 1);
    if (fread(anim->name, 1, nameLen, f) != nameLen) {
        fprintf(stderr, "Failed to read animation name\n");
        free_psa(anim);
        fclose(f);
        return NULL;
    }

    // Read frame length (unused but still in file)
    anim->frameLength = read_float(f);

    // Read animation data
    anim->numBones = read_u32(f);
    anim->numFrames = read_u32(f);
    
    // Validation: Check bone count limit (192 max according to PSAConvert.cpp)
    if (anim->numBones > 192) {
        fprintf(stderr, "Warning: Too many bones (%u > 192 max) - skeleton may have issues\n", anim->numBones);
    }

    anim->boneStates = calloc(anim->numBones * anim->numFrames, sizeof(BoneState));
    for (uint32_t i = 0; i < anim->numBones * anim->numFrames; i++) {
        anim->boneStates[i].translation = read_vec3(f);
        anim->boneStates[i].rotation = read_quat(f);
    }

    fclose(f);
    return anim;
}

void free_psa(PSAAnimation *anim) {
    if (!anim) return;
    free(anim->name);
    free(anim->boneStates);
    free(anim);
}

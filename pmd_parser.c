#include "pmd_psa_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read helpers for little-endian data
static uint32_t read_u32(FILE *f) {
    uint8_t buf[4];
    fread(buf, 1, 4, f);
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static uint16_t read_u16(FILE *f) {
    uint8_t buf[2];
    fread(buf, 1, 2, f);
    return buf[0] | (buf[1] << 8);
}

static uint8_t read_u8(FILE *f) {
    uint8_t val;
    fread(&val, 1, 1, f);
    return val;
}

static float read_float(FILE *f) {
    float val;
    fread(&val, sizeof(float), 1, f);
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

// Load PMD file
PMDModel* load_pmd(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return NULL;
    }

    PMDModel *model = calloc(1, sizeof(PMDModel));

    // Read header
    char magic[4];
    fread(magic, 1, 4, f);
    if (memcmp(magic, "PSMD", 4) != 0) {
        fprintf(stderr, "Invalid PMD magic\n");
        free(model);
        fclose(f);
        return NULL;
    }

    model->version = read_u32(f);
    uint32_t data_size = read_u32(f);

    // Read vertices
    model->numVertices = read_u32(f);
    model->numTexCoords = (model->version >= 4) ? read_u32(f) : 1;
    model->vertices = calloc(model->numVertices, sizeof(Vertex));

    for (uint32_t i = 0; i < model->numVertices; i++) {
        Vertex *v = &model->vertices[i];
        v->position = read_vec3(f);
        v->normal = read_vec3(f);

        v->coords = calloc(model->numTexCoords, sizeof(TexCoord));
        for (uint32_t j = 0; j < model->numTexCoords; j++) {
            v->coords[j].u = read_float(f);
            v->coords[j].v = read_float(f);
        }

        for (int j = 0; j < 4; j++) {
            v->blend.bones[j] = read_u8(f);
        }
        for (int j = 0; j < 4; j++) {
            v->blend.weights[j] = read_float(f);
        }
    }

    // Read faces
    model->numFaces = read_u32(f);
    model->faces = calloc(model->numFaces, sizeof(Face));
    for (uint32_t i = 0; i < model->numFaces; i++) {
        for (int j = 0; j < 3; j++) {
            model->faces[i].vertices[j] = read_u16(f);
        }
    }

    // Read bones
    model->numBones = read_u32(f);
    model->restStates = calloc(model->numBones, sizeof(BoneState));
    for (uint32_t i = 0; i < model->numBones; i++) {
        model->restStates[i].translation = read_vec3(f);
        model->restStates[i].rotation = read_quat(f);
    }

    // Read prop points (version 2+)
    if (model->version >= 2) {
        model->numPropPoints = read_u32(f);
        model->propPoints = calloc(model->numPropPoints, sizeof(PropPoint));
        for (uint32_t i = 0; i < model->numPropPoints; i++) {
            uint32_t nameLen = read_u32(f);
            model->propPoints[i].name = calloc(nameLen + 1, 1);
            fread(model->propPoints[i].name, 1, nameLen, f);
            model->propPoints[i].translation = read_vec3(f);
            model->propPoints[i].rotation = read_quat(f);
            model->propPoints[i].bone = read_u8(f);
        }
    }

    fclose(f);
    return model;
}

void free_pmd(PMDModel *model) {
    if (!model) return;

    if (model->vertices) {
        for (uint32_t i = 0; i < model->numVertices; i++) {
            free(model->vertices[i].coords);
        }
        free(model->vertices);
    }
    free(model->faces);
    free(model->restStates);

    if (model->propPoints) {
        for (uint32_t i = 0; i < model->numPropPoints; i++) {
            free(model->propPoints[i].name);
        }
        free(model->propPoints);
    }

    free(model);
}

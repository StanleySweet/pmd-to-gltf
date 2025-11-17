#include "pmd_writer.h"
#include <string.h>
#include <stdlib.h>

// Write helpers for little-endian data
static void write_u32(FILE *f, uint32_t val) {
    uint8_t buf[4];
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
    fwrite(buf, 1, 4, f);
}

static void write_u16(FILE *f, uint16_t val) {
    uint8_t buf[2];
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    fwrite(buf, 1, 2, f);
}

static void write_u8(FILE *f, uint8_t val) {
    fwrite(&val, 1, 1, f);
}

static void write_float(FILE *f, float val) {
    fwrite(&val, sizeof(float), 1, f);
}

static void write_vec3(FILE *f, Vector3D v) {
    write_float(f, v.x);
    write_float(f, v.y);
    write_float(f, v.z);
}

static void write_quat(FILE *f, Quaternion q) {
    write_float(f, q.x);
    write_float(f, q.y);
    write_float(f, q.z);
    write_float(f, q.w);
}

static void write_bone_state(FILE *f, BoneState state) {
    write_vec3(f, state.translation);
    write_quat(f, state.rotation);
}

int write_pmd(const char *filename, const PMDModel *model) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        return 0;
    }

    // Calculate data size
    uint32_t data_size = 0;
    data_size += 4;  // numVertices
    data_size += 4;  // numTexCoords
    data_size += model->numVertices * (3*4 + 3*4 + model->numTexCoords*2*4 + 4 + 4*4);  // vertices
    data_size += 4;  // numFaces
    data_size += model->numFaces * 3 * 2;  // faces
    data_size += 4;  // numBones
    data_size += model->numBones * (3*4 + 4*4);  // bone states
    data_size += 4;  // numPropPoints
    for (uint32_t i = 0; i < model->numPropPoints; i++) {
        data_size += 4;  // nameLength
        data_size += (uint32_t)strlen(model->propPoints[i].name);  // name
        data_size += 3*4 + 4*4 + 1;  // translation, rotation, bone
    }

    // Write header
    fwrite("PSMD", 1, 4, f);
    write_u32(f, model->version);
    write_u32(f, data_size);

    // Write vertices
    write_u32(f, model->numVertices);
    write_u32(f, model->numTexCoords);
    
    for (uint32_t i = 0; i < model->numVertices; i++) {
        Vertex *v = &model->vertices[i];
        write_vec3(f, v->position);
        write_vec3(f, v->normal);
        
        for (uint32_t j = 0; j < model->numTexCoords; j++) {
            write_float(f, v->coords[j].u);
            write_float(f, v->coords[j].v);
        }
        
        for (int j = 0; j < 4; j++) {
            write_u8(f, v->blend.bones[j]);
        }
        for (int j = 0; j < 4; j++) {
            write_float(f, v->blend.weights[j]);
        }
    }

    // Write faces
    write_u32(f, model->numFaces);
    for (uint32_t i = 0; i < model->numFaces; i++) {
        for (int j = 0; j < 3; j++) {
            write_u16(f, model->faces[i].vertices[j]);
        }
    }

    // Write bones
    write_u32(f, model->numBones);
    for (uint32_t i = 0; i < model->numBones; i++) {
        write_bone_state(f, model->restStates[i]);
    }

    // Write prop points
    write_u32(f, model->numPropPoints);
    for (uint32_t i = 0; i < model->numPropPoints; i++) {
        PropPoint *pp = &model->propPoints[i];
        uint32_t nameLen = (uint32_t)strlen(pp->name);
        write_u32(f, nameLen);
        fwrite(pp->name, 1, nameLen, f);
        write_vec3(f, pp->translation);
        write_quat(f, pp->rotation);
        write_u8(f, pp->bone);
    }

    fclose(f);
    return 1;
}

int write_psa(const char *filename, const PSAAnimation *anim) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        return 0;
    }

    // Calculate data size
    uint32_t nameLen = (uint32_t)strlen(anim->name);
    uint32_t data_size = 0;
    data_size += 4;  // nameLength
    data_size += nameLen;  // name
    data_size += 4;  // frameLength
    data_size += 4;  // numBones
    data_size += 4;  // numFrames
    data_size += anim->numBones * anim->numFrames * (3*4 + 4*4);  // bone states

    // Write header
    fwrite("PSSA", 1, 4, f);
    write_u32(f, 1);  // version
    write_u32(f, data_size);

    // Write animation data
    write_u32(f, nameLen);
    fwrite(anim->name, 1, nameLen, f);
    write_float(f, anim->frameLength);
    write_u32(f, anim->numBones);
    write_u32(f, anim->numFrames);

    // Write bone states
    for (uint32_t i = 0; i < anim->numBones * anim->numFrames; i++) {
        write_bone_state(f, anim->boneStates[i]);
    }

    fclose(f);
    return 1;
}

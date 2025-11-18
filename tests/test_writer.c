#include "portable_string.h"
#include <assert.h>
#define ASSERT(x) do { if (!(x)) { fprintf(stderr, "Assertion failed: %s (line %d)\n", #x, __LINE__); return 1; } } while(0)
#include "pmd_writer.h"
#include "pmd_psa_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("[TEST] Allocation PMDModel\n");
    PMDModel model = {0};
    model.version = 4;
    model.numVertices = 3;
    model.numTexCoords = 1;
    model.vertices = calloc(model.numVertices, sizeof(Vertex));
    ASSERT(model.vertices);
    for (uint32_t i = 0; i < model.numVertices; i++) {
        model.vertices[i].coords = calloc(model.numTexCoords, sizeof(TexCoord));
        ASSERT(model.vertices[i].coords);
        model.vertices[i].coords[0].u = 0.0f;
        model.vertices[i].coords[0].v = 0.0f;
        model.vertices[i].position.x = (float)i;
        model.vertices[i].position.y = 0.0f;
        model.vertices[i].position.z = 0.0f;
        model.vertices[i].normal.x = 0.0f;
        model.vertices[i].normal.y = 0.0f;
        model.vertices[i].normal.z = 1.0f;
        for (int j = 0; j < 4; j++) {
            model.vertices[i].blend.bones[j] = 0;
            model.vertices[i].blend.weights[j] = (j == 0) ? 1.0f : 0.0f;
        }
    }
    printf("[TEST] Allocation faces\n");
    model.numFaces = 1;
    model.faces = calloc(model.numFaces, sizeof(Face));
    ASSERT(model.faces);
    model.faces[0].vertices[0] = 0;
    model.faces[0].vertices[1] = 1;
    model.faces[0].vertices[2] = 2;
    printf("[TEST] Allocation bones\n");
    model.numBones = 1;
    model.restStates = calloc(model.numBones, sizeof(BoneState));
    ASSERT(model.restStates);
    model.restStates[0].translation.x = 0.0f;
    model.restStates[0].translation.y = 0.0f;
    model.restStates[0].translation.z = 0.0f;
    model.restStates[0].rotation.x = 0.0f;
    model.restStates[0].rotation.y = 0.0f;
    model.restStates[0].rotation.z = 0.0f;
    model.restStates[0].rotation.w = 1.0f;
    model.numPropPoints = 0;
    model.propPoints = NULL;

    printf("[TEST] Ajout du bone root\n");
    model.numBones = 2;
    free(model.restStates);
    model.restStates = calloc(model.numBones, sizeof(BoneState));
    ASSERT(model.restStates);
    // Bone 0 : root
    model.restStates[0].translation.x = 0.0f;
    model.restStates[0].translation.y = 0.0f;
    model.restStates[0].translation.z = 0.0f;
    model.restStates[0].rotation.x = 0.0f;
    model.restStates[0].rotation.y = 0.0f;
    model.restStates[0].rotation.z = 0.0f;
    model.restStates[0].rotation.w = 1.0f;
    // Bone 1 : comme avant
    model.restStates[1].translation.x = 1.0f;
    model.restStates[1].translation.y = 0.0f;
    model.restStates[1].translation.z = 0.0f;
    model.restStates[1].rotation.x = 0.0f;
    model.restStates[1].rotation.y = 0.0f;
    model.restStates[1].rotation.z = 0.0f;
    model.restStates[1].rotation.w = 1.0f;

    printf("[TEST] Ecriture PMD\n");
    int ok = write_pmd("tests/data/test_minimal.pmd", &model);
    ASSERT(ok && "Ecriture PMD échouée");
    for (uint32_t i = 0; i < model.numVertices; i++) {
        free(model.vertices[i].coords);
    }
    free(model.vertices);
    free(model.faces);
    free(model.restStates);

    printf("[TEST] Allocation PSAAnimation\n");
    PSAAnimation anim = {0};
    anim.name = my_strdup("test_anim");
    ASSERT(anim.name);
    anim.frameLength = 0.03333f;
    anim.numBones = 1;
    anim.numFrames = 1;
    anim.boneStates = calloc(anim.numBones * anim.numFrames, sizeof(BoneState));
    ASSERT(anim.boneStates);
    anim.boneStates[0].translation.x = 0.0f;
    anim.boneStates[0].translation.y = 0.0f;
    anim.boneStates[0].translation.z = 0.0f;
    anim.boneStates[0].rotation.x = 0.0f;
    anim.boneStates[0].rotation.y = 0.0f;
    anim.boneStates[0].rotation.z = 0.0f;
    anim.boneStates[0].rotation.w = 1.0f;

    printf("[TEST] Ecriture PSA\n");
    ok = write_psa("tests/data/test_minimal.psa", &anim);
    ASSERT(ok && "Ecriture PSA échouée");
    free(anim.name);
    free(anim.boneStates);

    printf("[OK] Tous les tests writer ont réussi.\n");
    return 0;
}

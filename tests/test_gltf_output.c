#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../vendor/cJSON/cJSON.h"
#include <unistd.h>
#include "pmd_writer.h"
#include "pmd_psa_types.h"

// Fonction utilitaire pour générer un cube PMD sans os
static void create_cube_nobones(const char *filename) {
    PMDModel model = {0};
    model.version = 4;
    model.numTexCoords = 1;
    model.numVertices = 8;
    model.numFaces = 12;
    model.vertices = calloc(model.numVertices, sizeof(Vertex));
    model.faces = calloc(model.numFaces, sizeof(Face));
    for (int i = 0; i < 8; i++) {
        Vertex *v = &model.vertices[i];
        v->position.x = (i & 1) ? 1.0f : -1.0f;
        v->position.y = (i & 2) ? 1.0f : -1.0f;
        v->position.z = (i & 4) ? 1.0f : -1.0f;
        v->normal.x = v->position.x;
        v->normal.y = v->position.y;
        v->normal.z = v->position.z;
        v->coords = calloc(1, sizeof(TexCoord));
        v->coords[0].u = (i % 2) ? 1.0f : 0.0f;
        v->coords[0].v = ((i / 2) % 2) ? 1.0f : 0.0f;
        for (int j = 0; j < 4; j++) {
            v->blend.bones[j] = 0xFF;
            v->blend.weights[j] = 0.0f;
        }
    }
    int faces[12][3] = {
        {0,1,3},{0,3,2},{4,6,7},{4,7,5},{0,2,6},{0,6,4},{1,5,7},{1,7,3},{0,4,5},{0,5,1},{2,3,7},{2,7,6}
    };
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 3; j++) model.faces[i].vertices[j] = faces[i][j];
    }
    model.numBones = 0;
    model.restStates = NULL;
    model.numPropPoints = 0;
    model.propPoints = NULL;
    write_pmd(filename, &model);
    free(model.vertices);
    free(model.faces);
}

static void export_cube_nobones_gltf(const char *pmd_file, const char *gltf_file) {
    PMDModel *model = load_pmd(pmd_file);
    export_gltf(gltf_file, model, NULL, 0, NULL, "cube_nobones", NULL, NULL);
    free_pmd(model);
}

// Helper to read file content
static char* read_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    size_t bytes_read = fread(content, 1, size, f);
    (void)bytes_read;  // Intentionally ignore result
    content[size] = '\0';
    fclose(f);
    
    return content;
}

// Test 1: Vérifie que le fichier glTF cube_nobones.gltf a été généré
static int test_gltf_nobones_exists(void) {
    // Génère le cube sans os en utilisant la logique de generate_test_data.c
    // On suppose que le fichier tests/data/cube_nobones.pmd existe déjà
    // On vérifie l'existence et la validité du fichier glTF
    char *content = read_file("tests/output/cube_nobones.gltf");
    TEST_ASSERT_NOT_NULL(content, "cube_nobones.gltf devrait exister");
    cJSON *root = cJSON_Parse(content);
    TEST_ASSERT_NOT_NULL(root, "Le JSON glTF doit être valide");
    cJSON *asset = cJSON_GetObjectItem(root, "asset");
    TEST_ASSERT_NOT_NULL(asset, "Le champ asset doit exister");
    cJSON *version = cJSON_GetObjectItem(asset, "version");
    TEST_ASSERT_NOT_NULL(version, "Le champ version doit exister");
    TEST_ASSERT(strcmp(version->valuestring, "2.0") == 0, "La version doit être 2.0");
    cJSON *meshes = cJSON_GetObjectItem(root, "meshes");
    TEST_ASSERT_NOT_NULL(meshes, "Le champ meshes doit exister");
    cJSON_Delete(root);
    free(content);
    return 1;
}

// Test 2: Vérifie que le fichier glTF cube_4bones.gltf a été généré
static int test_gltf_4bones_exists(void) {
    char *content = read_file("tests/output/cube_4bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "cube_4bones.gltf devrait exister");
    cJSON *root = cJSON_Parse(content);
    TEST_ASSERT_NOT_NULL(root, "Le JSON glTF doit être valide");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "skins"), "Le champ skins doit exister");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "animations"), "Le champ animations doit exister");
    cJSON *meshes = cJSON_GetObjectItem(root, "meshes");
    TEST_ASSERT_NOT_NULL(meshes, "Le champ meshes doit exister");
    cJSON *mesh = cJSON_GetArrayItem(meshes, 0);
    TEST_ASSERT_NOT_NULL(mesh, "Le mesh doit exister");
    cJSON *primitives = cJSON_GetObjectItem(mesh, "primitives");
    TEST_ASSERT_NOT_NULL(primitives, "Le champ primitives doit exister");
    cJSON *prim = cJSON_GetArrayItem(primitives, 0);
    TEST_ASSERT_NOT_NULL(prim, "Le primitive doit exister");
    cJSON *attributes = cJSON_GetObjectItem(prim, "attributes");
    TEST_ASSERT_NOT_NULL(attributes, "Le champ attributes doit exister");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(attributes, "JOINTS_0"), "Le champ JOINTS_0 doit exister");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(attributes, "WEIGHTS_0"), "Le champ WEIGHTS_0 doit exister");
    cJSON_Delete(root);
    free(content);
    return 1;
}

// Test 3: Verify glTF file for 5 bones hierarchical cube
static int test_gltf_5bones_exists(void) {
    char *content = read_file("tests/output/cube_5bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "cube_5bones.gltf should exist");
    cJSON *root = cJSON_Parse(content);
    TEST_ASSERT_NOT_NULL(root, "glTF JSON should parse");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "nodes"), "Should have nodes");
    // Check for children in nodes
    cJSON *nodes = cJSON_GetObjectItem(root, "nodes");
    TEST_ASSERT_NOT_NULL(nodes, "Should have nodes");
    int has_children = 0;
    cJSON *node = NULL;
    cJSON_ArrayForEach(node, nodes) {
        if (cJSON_GetObjectItem(node, "children")) {
            has_children = 1;
            break;
        }
    }
    TEST_ASSERT(has_children, "Should have node children");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "skins"), "Should have skins");
    cJSON_Delete(root);
    free(content);
    return 1;
}

// Test 4: Verify cube has correct number of vertices in glTF
static int test_gltf_vertex_count(void) {
    char *content = read_file("tests/output/cube_nobones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    cJSON *root = cJSON_Parse(content);
    TEST_ASSERT_NOT_NULL(root, "glTF JSON should parse");
    cJSON *accessors = cJSON_GetObjectItem(root, "accessors");
    TEST_ASSERT_NOT_NULL(accessors, "Should have accessors");
    cJSON *pos_accessor = NULL;
    cJSON_ArrayForEach(pos_accessor, accessors) {
        cJSON *type = cJSON_GetObjectItem(pos_accessor, "type");
        if (type && strcmp(type->valuestring, "VEC3") == 0) {
            break;
        }
    }
    TEST_ASSERT_NOT_NULL(pos_accessor, "Should find position accessor");
    cJSON *count = cJSON_GetObjectItem(pos_accessor, "count");
    TEST_ASSERT_NOT_NULL(count, "Position accessor should have count");
    TEST_ASSERT_EQ(8, count->valueint, "Should have 8 vertices");
    cJSON_Delete(root);
    free(content);
    return 1;
}

// Test 5: Verify mesh name is preserved
static int test_gltf_mesh_name(void) {
    char *content = read_file("tests/output/cube_nobones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    cJSON *root = cJSON_Parse(content);
    TEST_ASSERT_NOT_NULL(root, "glTF JSON should parse");
    cJSON *meshes = cJSON_GetObjectItem(root, "meshes");
    TEST_ASSERT_NOT_NULL(meshes, "Should have meshes");
    cJSON *mesh = cJSON_GetArrayItem(meshes, 0);
    TEST_ASSERT_NOT_NULL(mesh, "Should have mesh");
    cJSON *name = cJSON_GetObjectItem(mesh, "name");
    TEST_ASSERT_NOT_NULL(name, "Mesh should have name");
    TEST_ASSERT(strcmp(name->valuestring, "cube_nobones") == 0, "Mesh should be named cube_nobones");
    cJSON_Delete(root);
    free(content);
    return 1;
}

// Test 6: Verify all glTF files are valid JSON
static int test_gltf_valid_json(void) {
    const char *files[] = {
        "tests/output/cube_nobones.gltf",
        "tests/output/cube_4bones.gltf",
        "tests/output/cube_5bones.gltf"
    };
    for (int i = 0; i < 3; i++) {
        char *content = read_file(files[i]);
        TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
        cJSON *root = cJSON_Parse(content);
        TEST_ASSERT_NOT_NULL(root, "JSON should parse");
        cJSON_Delete(root);
        free(content);
    }
    return 1;
}

// Test 7: Verify required glTF 2.0 fields are present
static int test_gltf_required_fields(void) {
    char *content = read_file("tests/output/cube_4bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    cJSON *root = cJSON_Parse(content);
    TEST_ASSERT_NOT_NULL(root, "glTF JSON should parse");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "asset"), "Must have asset");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "scenes"), "Should have scenes");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "nodes"), "Should have nodes");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "meshes"), "Should have meshes");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "accessors"), "Should have accessors");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "bufferViews"), "Should have bufferViews");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "buffers"), "Should have buffers");
    cJSON_Delete(root);
    free(content);
    return 1;
}

// Test 8: Verify animation is exported
static int test_gltf_animation_export(void) {
    char *content = read_file("tests/output/cube_4bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    cJSON *root = cJSON_Parse(content);
    TEST_ASSERT_NOT_NULL(root, "glTF JSON should parse");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(root, "animations"), "Should have animations array");
    cJSON *animations = cJSON_GetObjectItem(root, "animations");
    cJSON *anim = cJSON_GetArrayItem(animations, 0);
    TEST_ASSERT_NOT_NULL(anim, "Should have animation");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(anim, "samplers"), "Should have animation samplers");
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(anim, "channels"), "Should have animation channels");
    cJSON_Delete(root);
    free(content);
    return 1;
}

static void create_cube_4bones(const char *filename) {
    PMDModel model = {0};
    model.version = 4;
    model.numTexCoords = 1;
    model.numVertices = 8;
    model.numFaces = 12;
    model.vertices = calloc(model.numVertices, sizeof(Vertex));
    model.faces = calloc(model.numFaces, sizeof(Face));
    for (int i = 0; i < 8; i++) {
        Vertex *v = &model.vertices[i];
        v->position.x = (i & 1) ? 1.0f : -1.0f;
        v->position.y = (i & 2) ? 1.0f : -1.0f;
        v->position.z = (i & 4) ? 1.0f : -1.0f;
        v->normal.x = v->position.x;
        v->normal.y = v->position.y;
        v->normal.z = v->position.z;
        v->coords = calloc(1, sizeof(TexCoord));
        v->coords[0].u = (i % 2) ? 1.0f : 0.0f;
        v->coords[0].v = ((i / 2) % 2) ? 1.0f : 0.0f;
        v->blend.bones[0] = i % 4;
        v->blend.weights[0] = 1.0f;
        for (int j = 1; j < 4; j++) {
            v->blend.bones[j] = 0xFF;
            v->blend.weights[j] = 0.0f;
        }
    }
    int faces[12][3] = {
        {0,1,3},{0,3,2},{4,6,7},{4,7,5},{0,2,6},{0,6,4},{1,5,7},{1,7,3},{0,4,5},{0,5,1},{2,3,7},{2,7,6}
    };
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 3; j++) model.faces[i].vertices[j] = faces[i][j];
    }
    model.numBones = 4;
    model.restStates = calloc(4, sizeof(BoneState));
    for (int i = 0; i < 4; i++) {
        model.restStates[i].translation.x = (i == 0 || i == 2) ? -1.0f : 1.0f;
        model.restStates[i].translation.y = (i == 0 || i == 1) ? -1.0f : 1.0f;
        model.restStates[i].translation.z = -1.0f;
        model.restStates[i].rotation.x = 0.0f;
        model.restStates[i].rotation.y = 0.0f;
        model.restStates[i].rotation.z = 0.0f;
        model.restStates[i].rotation.w = 1.0f;
    }
    model.numPropPoints = 0;
    model.propPoints = NULL;
    write_pmd(filename, &model);
    free(model.vertices);
    free(model.faces);
    free(model.restStates);
}

static void create_cube_5bones(const char *filename) {
    PMDModel model = {0};
    model.version = 4;
    model.numTexCoords = 1;
    model.numVertices = 8;
    model.numFaces = 12;
    model.vertices = calloc(model.numVertices, sizeof(Vertex));
    model.faces = calloc(model.numFaces, sizeof(Face));
    for (int i = 0; i < 8; i++) {
        Vertex *v = &model.vertices[i];
        v->position.x = (i & 1) ? 1.0f : -1.0f;
        v->position.y = (i & 2) ? 1.0f : -1.0f;
        v->position.z = (i & 4) ? 1.0f : -1.0f;
        v->normal.x = v->position.x;
        v->normal.y = v->position.y;
        v->normal.z = v->position.z;
        v->coords = calloc(1, sizeof(TexCoord));
        v->coords[0].u = (i % 2) ? 1.0f : 0.0f;
        v->coords[0].v = ((i / 2) % 2) ? 1.0f : 0.0f;
        if (i < 4) {
            v->blend.bones[0] = i;
            v->blend.weights[0] = 1.0f;
            v->blend.bones[1] = 0xFF;
            v->blend.weights[1] = 0.0f;
        } else {
            int corner = i - 4;
            v->blend.bones[0] = corner;
            v->blend.weights[0] = 0.5f;
            v->blend.bones[1] = 4;
            v->blend.weights[1] = 0.5f;
        }
        v->blend.bones[2] = 0xFF;
        v->blend.weights[2] = 0.0f;
        v->blend.bones[3] = 0xFF;
        v->blend.weights[3] = 0.0f;
    }
    int faces[12][3] = {
        {0,1,3},{0,3,2},{4,6,7},{4,7,5},{0,2,6},{0,6,4},{1,5,7},{1,7,3},{0,4,5},{0,5,1},{2,3,7},{2,7,6}
    };
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 3; j++) model.faces[i].vertices[j] = faces[i][j];
    }
    model.numBones = 5;
    model.restStates = calloc(5, sizeof(BoneState));
    for (int i = 0; i < 4; i++) {
        model.restStates[i].translation.x = (i == 0 || i == 2) ? -1.0f : 1.0f;
        model.restStates[i].translation.y = (i == 0 || i == 1) ? -1.0f : 1.0f;
        model.restStates[i].translation.z = -1.0f;
        model.restStates[i].rotation.x = 0.0f;
        model.restStates[i].rotation.y = 0.0f;
        model.restStates[i].rotation.z = 0.0f;
        model.restStates[i].rotation.w = 1.0f;
    }
    model.restStates[4].translation.x = 0.0f;
    model.restStates[4].translation.y = 0.0f;
    model.restStates[4].translation.z = 0.0f;
    model.restStates[4].rotation.x = 0.0f;
    model.restStates[4].rotation.y = 0.0f;
    model.restStates[4].rotation.z = 0.0f;
    model.restStates[4].rotation.w = 1.0f;
    model.numPropPoints = 0;
    model.propPoints = NULL;
    write_pmd(filename, &model);
    free(model.vertices);
    free(model.faces);
    free(model.restStates);
}

static PSAAnimation* create_simple_4bones_anim(void) {
    PSAAnimation *anim = calloc(1, sizeof(PSAAnimation));
    anim->name = strdup("test_anim");
    anim->frameLength = 0.03333f;  // 30 fps
    anim->numBones = 4;
    anim->numFrames = 10;
    anim->boneStates = calloc(4 * 10, sizeof(BoneState));
    for (uint32_t frame = 0; frame < 10; frame++) {
        float t = (float)frame / 9.0f;
        float angle = t * 3.14159f * 2.0f;
        for (int bone = 0; bone < 4; bone++) {
            int idx = frame * 4 + bone;
            anim->boneStates[idx].translation.x = (bone == 0 || bone == 2) ? -1.0f : 1.0f;
            anim->boneStates[idx].translation.y = (bone == 0 || bone == 1) ? -1.0f : 1.0f;
            anim->boneStates[idx].translation.z = -1.0f;
            anim->boneStates[idx].rotation.x = 0.0f;
            anim->boneStates[idx].rotation.y = 0.0f;
            anim->boneStates[idx].rotation.z = sinf(angle/2.0f);
            anim->boneStates[idx].rotation.w = cosf(angle/2.0f);
        }
    }
    return anim;
}

static void export_cube_4bones_gltf(const char *pmd_file, const char *gltf_file) {
    PMDModel *model = load_pmd(pmd_file);
    PSAAnimation *anim = create_simple_4bones_anim();
    PSAAnimation *anims[1] = {anim};
    export_gltf(gltf_file, model, anims, 1, NULL, "cube_4bones", NULL, NULL);
    free_psa(anim);
    free_pmd(model);
}

static void export_cube_5bones_gltf(const char *pmd_file, const char *gltf_file) {
    PMDModel *model = load_pmd(pmd_file);
    export_gltf(gltf_file, model, NULL, 0, NULL, "cube_5bones", NULL, NULL);
    free_pmd(model);
}

int main(void) {
    // Générer les PMD et glTF nécessaires dans tests/output
    create_cube_nobones("tests/output/cube_nobones.pmd");
    export_cube_nobones_gltf("tests/output/cube_nobones.pmd", "tests/output/cube_nobones.gltf");
    create_cube_4bones("tests/output/cube_4bones.pmd");
    export_cube_4bones_gltf("tests/output/cube_4bones.pmd", "tests/output/cube_4bones.gltf");
    create_cube_5bones("tests/output/cube_5bones.pmd");
    export_cube_5bones_gltf("tests/output/cube_5bones.pmd", "tests/output/cube_5bones.gltf");
    const test_case_t tests[] = {
        {"gltf_nobones_exists", test_gltf_nobones_exists},
        {"gltf_4bones_exists", test_gltf_4bones_exists},
        {"gltf_5bones_exists", test_gltf_5bones_exists},
        {"gltf_vertex_count", test_gltf_vertex_count},
        {"gltf_mesh_name", test_gltf_mesh_name},
        {"gltf_valid_json", test_gltf_valid_json},
        {"gltf_required_fields", test_gltf_required_fields},
        {"gltf_animation_export", test_gltf_animation_export}
    };
    int result = run_tests(tests, sizeof(tests) / sizeof(tests[0]));
    // Nettoyage des fichiers générés
    // Nettoyage cross-platform des fichiers générés
    const char *files_to_remove[] = {
        "tests/output/cube_nobones.gltf",
        "tests/output/cube_4bones.gltf",
        "tests/output/cube_5bones.gltf",
        "tests/output/cube_2bones_2props.gltf",
        "tests/output/cube_nobones.pmd",
        "tests/output/cube_4bones.pmd",
        "tests/output/cube_5bones.pmd",
        "tests/output/cube_2bones_2props.pmd",
        "tests/output/cube_nobones.psa",
        "tests/output/cube_4bones.psa",
        "tests/output/cube_5bones.psa",
        "tests/output/cube_2bones_2props.psa"
    };
    for (size_t i = 0; i < sizeof(files_to_remove)/sizeof(files_to_remove[0]); ++i) {
        remove(files_to_remove[i]); // ignore l'erreur si le fichier n'existe pas
    }
    return result;
}

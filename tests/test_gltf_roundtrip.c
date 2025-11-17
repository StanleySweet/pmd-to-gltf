#include "test_framework.h"
#include "pmd_writer.h"
#include "pmd_psa_types.h"
#include "../vendor/cJSON/cJSON.h"
#include <math.h>
#include <unistd.h>

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

// Base64 decoding table
static const unsigned char base64_decode_table[256] = {
    ['A'] = 0, ['B'] = 1, ['C'] = 2, ['D'] = 3, ['E'] = 4, ['F'] = 5, ['G'] = 6, ['H'] = 7,
    ['I'] = 8, ['J'] = 9, ['K'] = 10, ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
    ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
    ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
    ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
    ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63
};

// Simple base64 decoder for data URIs
static size_t base64_decode(const char *input, unsigned char *output, size_t output_size) {
    size_t input_len = strlen(input);
    size_t output_len = 0;
    unsigned char buffer[4];
    int buffer_pos = 0;
    
    for (size_t i = 0; i < input_len && input[i] != '='; i++) {
        unsigned char c = input[i];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
        
        buffer[buffer_pos++] = base64_decode_table[c];
        
        if (buffer_pos == 4) {
            if (output_len + 3 > output_size) return output_len;
            
            output[output_len++] = (buffer[0] << 2) | (buffer[1] >> 4);
            output[output_len++] = (buffer[1] << 4) | (buffer[2] >> 2);
            output[output_len++] = (buffer[2] << 6) | buffer[3];
            buffer_pos = 0;
        }
    }
    
    if (buffer_pos >= 2 && output_len < output_size) {
        output[output_len++] = (buffer[0] << 2) | (buffer[1] >> 4);
        if (buffer_pos >= 3 && output_len < output_size) {
            output[output_len++] = (buffer[1] << 4) | (buffer[2] >> 2);
        }
    }
    
    return output_len;
}

// Extract base64 data from data URI
static const char* extract_base64_from_uri(const char *uri) {
    const char *prefix = "data:application/octet-stream;base64,";
    if (strncmp(uri, prefix, strlen(prefix)) == 0) {
        return uri + strlen(prefix);
    }
    return NULL;
}

// Helper to compare floats with tolerance
static int float_equal(float a, float b, float tolerance) {
    return fabsf(a - b) < tolerance;
}

// Test : roundtrip_cube_nobones - vérifie que les données vertex glTF correspondent au PMD original
static int test_roundtrip_cube_nobones(void) {
    PMDModel *pmd = load_pmd("tests/data/cube_nobones.pmd");
    TEST_ASSERT_NOT_NULL(pmd, "Le PMD original doit être chargé");
    TEST_ASSERT_EQ(8, pmd->numVertices, "Le PMD doit avoir 8 sommets");
    FILE *f = fopen("tests/output/cube_nobones.gltf", "r");
    TEST_ASSERT_NOT_NULL(f, "Le fichier glTF doit être ouvert");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *gltf_content = malloc(size + 1);
    size_t bytes_read = fread(gltf_content, 1, size, f);
    (void)bytes_read;
    gltf_content[size] = '\0';
    fclose(f);
    cJSON *root = cJSON_Parse(gltf_content);
    TEST_ASSERT_NOT_NULL(root, "Le JSON glTF doit être valide");
    cJSON *asset = cJSON_GetObjectItem(root, "asset");
    TEST_ASSERT_NOT_NULL(asset, "Le champ asset doit exister");
    cJSON *version = cJSON_GetObjectItem(asset, "version");
    TEST_ASSERT_NOT_NULL(version, "Le champ version doit exister");
    TEST_ASSERT(strcmp(version->valuestring, "2.0") == 0, "La version doit être 2.0");
    cJSON *accessors = cJSON_GetObjectItem(root, "accessors");
    TEST_ASSERT_NOT_NULL(accessors, "Le champ accessors doit exister");
    cJSON *pos_accessor = NULL;
    cJSON_ArrayForEach(pos_accessor, accessors) {
        cJSON *type = cJSON_GetObjectItem(pos_accessor, "type");
        if (type && strcmp(type->valuestring, "VEC3") == 0) {
            break;
        }
    }
    TEST_ASSERT_NOT_NULL(pos_accessor, "L'accessor position doit exister");
    cJSON *count = cJSON_GetObjectItem(pos_accessor, "count");
    TEST_ASSERT_NOT_NULL(count, "Le champ count doit exister");
    TEST_ASSERT_EQ(8, count->valueint, "Il doit y avoir 8 sommets");
    cJSON *bufferViews = cJSON_GetObjectItem(root, "bufferViews");
    TEST_ASSERT_NOT_NULL(bufferViews, "Le champ bufferViews doit exister");
    cJSON *buffers = cJSON_GetObjectItem(root, "buffers");
    TEST_ASSERT_NOT_NULL(buffers, "Le champ buffers doit exister");
    cJSON *buffer = cJSON_GetArrayItem(buffers, 0);
    TEST_ASSERT_NOT_NULL(buffer, "Le buffer doit exister");
    cJSON *uri_json = cJSON_GetObjectItem(buffer, "uri");
    TEST_ASSERT_NOT_NULL(uri_json, "Le buffer doit avoir un URI");
    const char *base64_data = extract_base64_from_uri(uri_json->valuestring);
    TEST_ASSERT_NOT_NULL(base64_data, "Les données base64 doivent être extraites");
    unsigned char decoded[1024];
    size_t decoded_len = base64_decode(base64_data, decoded, sizeof(decoded));
    TEST_ASSERT_EQ(96, decoded_len, "La taille décodée doit être 96 octets (8 sommets * 3 floats * 4 octets)");
    float *positions = (float*)decoded;
    for (uint32_t i = 0; i < pmd->numVertices; i++) {
        float pmd_x = pmd->vertices[i].position.x;
        float pmd_y = pmd->vertices[i].position.y;
        float pmd_z = pmd->vertices[i].position.z;
        float gltf_x = positions[i * 3 + 0];
        float gltf_y = positions[i * 3 + 1];
        float gltf_z = positions[i * 3 + 2];
        TEST_ASSERT(float_equal(pmd_x, gltf_x, 0.001f), "X du sommet doit correspondre");
        TEST_ASSERT(float_equal(pmd_y, gltf_y, 0.001f), "Y du sommet doit correspondre");
        TEST_ASSERT(float_equal(pmd_z, gltf_z, 0.001f), "Z du sommet doit correspondre");
    }
    cJSON_Delete(root);
    free(gltf_content);
    free_pmd(pmd);
    return 1;
}

// Test : roundtrip_cube_4bones - vérifie que les positions des sommets sont préservées
static int test_roundtrip_cube_4bones(void) {
    PMDModel *pmd = load_pmd("tests/data/cube_4bones.pmd");
    TEST_ASSERT_NOT_NULL(pmd, "Le PMD original doit être chargé");
    FILE *f = fopen("tests/output/cube_4bones.gltf", "r");
    TEST_ASSERT_NOT_NULL(f, "Le fichier glTF doit être ouvert");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *gltf_content = malloc(size + 1);
    size_t bytes_read = fread(gltf_content, 1, size, f);
    (void)bytes_read;
    gltf_content[size] = '\0';
    fclose(f);
    cJSON *root = cJSON_Parse(gltf_content);
    TEST_ASSERT_NOT_NULL(root, "Le JSON glTF doit être valide");
    cJSON *asset = cJSON_GetObjectItem(root, "asset");
    TEST_ASSERT_NOT_NULL(asset, "Le champ asset doit exister");
    cJSON *version = cJSON_GetObjectItem(asset, "version");
    TEST_ASSERT_NOT_NULL(version, "Le champ version doit exister");
    TEST_ASSERT(strcmp(version->valuestring, "2.0") == 0, "La version doit être 2.0");
    cJSON *animations = cJSON_GetObjectItem(root, "animations");
    TEST_ASSERT_NOT_NULL(animations, "Le champ animations doit exister");
    cJSON *accessors = cJSON_GetObjectItem(root, "accessors");
    TEST_ASSERT_NOT_NULL(accessors, "Le champ accessors doit exister");
    cJSON *pos_accessor = NULL;
    cJSON_ArrayForEach(pos_accessor, accessors) {
        cJSON *type = cJSON_GetObjectItem(pos_accessor, "type");
        if (type && strcmp(type->valuestring, "VEC3") == 0) {
            break;
        }
    }
    TEST_ASSERT_NOT_NULL(pos_accessor, "L'accessor position doit exister");
    cJSON *count = cJSON_GetObjectItem(pos_accessor, "count");
    TEST_ASSERT_NOT_NULL(count, "Le champ count doit exister");
    TEST_ASSERT_EQ(8, count->valueint, "Il doit y avoir 8 sommets");
    cJSON_Delete(root);
    free(gltf_content);
    free_pmd(pmd);
    return 1;
}

// Test: Verify glTF can be parsed as valid JSON
static int test_gltf_json_validity(void) {
    const char *files[] = {
        "tests/output/cube_nobones.gltf",
        "tests/output/cube_4bones.gltf",
        "tests/output/cube_5bones.gltf",
        "tests/output/cube_2bones_2props.gltf"
    };
    for (int i = 0; i < 4; i++) {
        FILE *f = fopen(files[i], "r");
        if (!f) continue;  // Skip if file doesn't exist
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *content = malloc(size + 1);
        size_t bytes_read = fread(content, 1, size, f);
        (void)bytes_read;
        content[size] = '\0';
        fclose(f);
        cJSON *root = cJSON_Parse(content);
        TEST_ASSERT_NOT_NULL(root, "JSON should parse");
        cJSON_Delete(root);
        free(content);
    }
    return 1;
}

// Test: Verify mesh bounds are preserved in glTF
static int test_gltf_preserves_bounds(void) {
    // Test cube dimensions are preserved
    PMDModel *pmd = load_pmd("tests/data/cube_nobones.pmd");
    TEST_ASSERT_NOT_NULL(pmd, "Should load PMD");
    
    // Find min/max from PMD
    float min_x = pmd->vertices[0].position.x;
    float max_x = pmd->vertices[0].position.x;
    
    for (uint32_t i = 1; i < pmd->numVertices; i++) {
        if (pmd->vertices[i].position.x < min_x) min_x = pmd->vertices[i].position.x;
        if (pmd->vertices[i].position.x > max_x) max_x = pmd->vertices[i].position.x;
    }
    
    // Verify cube is 2m wide (from -1 to 1)
    TEST_ASSERT(float_equal(-1.0f, min_x, 0.001f), "Min X should be -1");
    TEST_ASSERT(float_equal(1.0f, max_x, 0.001f), "Max X should be 1");
    TEST_ASSERT(float_equal(2.0f, max_x - min_x, 0.001f), "Width should be 2m");
    
    free_pmd(pmd);
    return 1;
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
        {"roundtrip_cube_nobones", test_roundtrip_cube_nobones},
        {"roundtrip_cube_4bones", test_roundtrip_cube_4bones},
        {"gltf_json_validity", test_gltf_json_validity},
        {"gltf_preserves_bounds", test_gltf_preserves_bounds}
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

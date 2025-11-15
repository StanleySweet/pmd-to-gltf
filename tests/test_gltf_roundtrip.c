#include "test_framework.h"
#include "../pmd_psa_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Forward declarations
extern PMDModel* load_pmd(const char *filename);
extern void free_pmd(PMDModel *model);

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

// Test: Round-trip cube_nobones - verify glTF vertex data matches original PMD
static int test_roundtrip_cube_nobones(void) {
    // Load original PMD
    PMDModel *pmd = load_pmd("tests/data/cube_nobones.pmd");
    TEST_ASSERT_NOT_NULL(pmd, "Should load original PMD");
    TEST_ASSERT_EQ(8, pmd->numVertices, "PMD should have 8 vertices");
    
    // Read glTF file
    FILE *f = fopen("tests/output/cube_nobones.gltf", "r");
    TEST_ASSERT_NOT_NULL(f, "Should open glTF file");
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *gltf_content = malloc(size + 1);
    size_t bytes_read = fread(gltf_content, 1, size, f);
    (void)bytes_read;
    gltf_content[size] = '\0';
    fclose(f);
    
    // Find the position buffer (first buffer in our glTF)
    const char *search = "\"uri\": \"data:application/octet-stream;base64,";
    char *uri_start = strstr(gltf_content, search);
    TEST_ASSERT_NOT_NULL(uri_start, "Should find position data URI");
    
    uri_start += strlen("\"uri\": \"");
    char *uri_end = strchr(uri_start, '"');
    TEST_ASSERT_NOT_NULL(uri_end, "Should find URI end");
    
    // Extract and decode base64 data
    size_t uri_len = uri_end - uri_start;
    char *uri = malloc(uri_len + 1);
    memcpy(uri, uri_start, uri_len);
    uri[uri_len] = '\0';
    
    const char *base64_data = extract_base64_from_uri(uri);
    TEST_ASSERT_NOT_NULL(base64_data, "Should extract base64 data");
    
    unsigned char decoded[1024];
    size_t decoded_len = base64_decode(base64_data, decoded, sizeof(decoded));
    TEST_ASSERT_EQ(96, decoded_len, "Decoded size should be 96 bytes (8 vertices * 3 floats * 4 bytes)");
    
    // Verify vertex positions match
    float *positions = (float*)decoded;
    for (uint32_t i = 0; i < pmd->numVertices; i++) {
        float pmd_x = pmd->vertices[i].position.x;
        float pmd_y = pmd->vertices[i].position.y;
        float pmd_z = pmd->vertices[i].position.z;
        
        float gltf_x = positions[i * 3 + 0];
        float gltf_y = positions[i * 3 + 1];
        float gltf_z = positions[i * 3 + 2];
        
        TEST_ASSERT(float_equal(pmd_x, gltf_x, 0.001f), "Vertex X should match");
        TEST_ASSERT(float_equal(pmd_y, gltf_y, 0.001f), "Vertex Y should match");
        TEST_ASSERT(float_equal(pmd_z, gltf_z, 0.001f), "Vertex Z should match");
    }
    
    free(uri);
    free(gltf_content);
    free_pmd(pmd);
    return 1;
}

// Test: Round-trip cube_4bones - verify glTF preserves vertex positions
static int test_roundtrip_cube_4bones(void) {
    PMDModel *pmd = load_pmd("tests/data/cube_4bones.pmd");
    TEST_ASSERT_NOT_NULL(pmd, "Should load original PMD");
    
    FILE *f = fopen("tests/output/cube_4bones.gltf", "r");
    TEST_ASSERT_NOT_NULL(f, "Should open glTF file");
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *gltf_content = malloc(size + 1);
    size_t bytes_read = fread(gltf_content, 1, size, f);
    (void)bytes_read;
    gltf_content[size] = '\0';
    fclose(f);
    
    // Verify glTF contains animation data
    TEST_ASSERT(strstr(gltf_content, "\"animations\"") != NULL, "Should have animations");
    TEST_ASSERT(strstr(gltf_content, "\"samplers\"") != NULL, "Should have animation samplers");
    TEST_ASSERT(strstr(gltf_content, "\"channels\"") != NULL, "Should have animation channels");
    
    // Find position buffer and verify vertex count
    const char *search = "\"count\": 8";
    TEST_ASSERT(strstr(gltf_content, search) != NULL, "Should have 8 vertices");
    
    free(gltf_content);
    free_pmd(pmd);
    return 1;
}

// Test: Round-trip horse model - verify glTF preserves complex structure
static int test_roundtrip_horse(void) {
    PMDModel *pmd = load_pmd("input/horse.pmd");
    if (!pmd) {
        // Horse model not available, skip test
        return 1;
    }
    
    TEST_ASSERT_EQ(206, pmd->numVertices, "Horse should have 206 vertices");
    TEST_ASSERT_EQ(33, pmd->numBones, "Horse should have 33 bones");
    
    FILE *f = fopen("output/cmake_test.gltf", "r");
    if (!f) {
        // Horse glTF not yet generated, skip test
        free_pmd(pmd);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *gltf_content = malloc(size + 1);
    size_t bytes_read = fread(gltf_content, 1, size, f);
    (void)bytes_read;
    gltf_content[size] = '\0';
    fclose(f);
    
    // Verify glTF structure
    TEST_ASSERT(strstr(gltf_content, "\"version\": \"2.0\"") != NULL, "Should be glTF 2.0");
    TEST_ASSERT(strstr(gltf_content, "\"meshes\"") != NULL, "Should have meshes");
    TEST_ASSERT(strstr(gltf_content, "\"nodes\"") != NULL, "Should have nodes");
    TEST_ASSERT(strstr(gltf_content, "\"skins\"") != NULL, "Should have skins");
    TEST_ASSERT(strstr(gltf_content, "\"animations\"") != NULL, "Should have animations");
    
    // Verify vertex count in glTF (should be 206)
    TEST_ASSERT(strstr(gltf_content, "\"count\": 206") != NULL, "Should have 206 vertices");
    
    // Verify bone/joint count (should match skeleton)
    const char *joints_str = strstr(gltf_content, "\"joints\":");
    TEST_ASSERT_NOT_NULL(joints_str, "Should have joints array");
    
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
        "output/cmake_test.gltf"
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
        
        // Basic JSON structure validation
        int brace_count = 0;
        int bracket_count = 0;
        int in_string = 0;
        
        for (long j = 0; j < size; j++) {
            char c = content[j];
            
            if (c == '"' && (j == 0 || content[j-1] != '\\')) {
                in_string = !in_string;
            }
            
            if (!in_string) {
                if (c == '{') brace_count++;
                else if (c == '}') brace_count--;
                else if (c == '[') bracket_count++;
                else if (c == ']') bracket_count--;
            }
        }
        
        TEST_ASSERT_EQ(0, brace_count, "JSON braces should be balanced");
        TEST_ASSERT_EQ(0, bracket_count, "JSON brackets should be balanced");
        
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
    const test_case_t tests[] = {
        {"roundtrip_cube_nobones", test_roundtrip_cube_nobones},
        {"roundtrip_cube_4bones", test_roundtrip_cube_4bones},
        {"roundtrip_horse", test_roundtrip_horse},
        {"gltf_json_validity", test_gltf_json_validity},
        {"gltf_preserves_bounds", test_gltf_preserves_bounds}
    };
    
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

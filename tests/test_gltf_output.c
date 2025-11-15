#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

// Test 1: Verify glTF file was created for no-bones cube
static int test_gltf_nobones_exists(void) {
    char *content = read_file("tests/output/cube_nobones.gltf");
    TEST_ASSERT_NOT_NULL(content, "cube_nobones.gltf should exist");
    
    // Check for required glTF fields
    TEST_ASSERT(strstr(content, "\"asset\"") != NULL, "Should have asset field");
    TEST_ASSERT(strstr(content, "\"version\": \"2.0\"") != NULL, "Should be glTF 2.0");
    TEST_ASSERT(strstr(content, "\"meshes\"") != NULL, "Should have meshes");
    
    free(content);
    return 1;
}

// Test 2: Verify glTF file for 4 bones cube
static int test_gltf_4bones_exists(void) {
    char *content = read_file("tests/output/cube_4bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "cube_4bones.gltf should exist");
    
    // Check for skeleton-related fields
    TEST_ASSERT(strstr(content, "\"skins\"") != NULL, "Should have skins");
    TEST_ASSERT(strstr(content, "\"animations\"") != NULL, "Should have animations");
    TEST_ASSERT(strstr(content, "\"JOINTS_0\"") != NULL, "Should have joint attributes");
    TEST_ASSERT(strstr(content, "\"WEIGHTS_0\"") != NULL, "Should have weight attributes");
    
    free(content);
    return 1;
}

// Test 3: Verify glTF file for 5 bones hierarchical cube
static int test_gltf_5bones_exists(void) {
    char *content = read_file("tests/output/cube_5bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "cube_5bones.gltf should exist");
    
    // Check for hierarchical structure
    TEST_ASSERT(strstr(content, "\"nodes\"") != NULL, "Should have nodes");
    TEST_ASSERT(strstr(content, "\"children\"") != NULL, "Should have node children");
    TEST_ASSERT(strstr(content, "\"skins\"") != NULL, "Should have skins");
    
    free(content);
    return 1;
}

// Test 4: Verify cube has correct number of vertices in glTF
static int test_gltf_vertex_count(void) {
    char *content = read_file("tests/output/cube_nobones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    
    // Look for accessor with count: 8 (vertices)
    // This is a simple check - full JSON parsing would be more robust
    TEST_ASSERT(strstr(content, "\"count\": 8") != NULL, "Should have 8 vertices");
    
    free(content);
    return 1;
}

// Test 5: Verify mesh name is preserved
static int test_gltf_mesh_name(void) {
    char *content = read_file("tests/output/cube_nobones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    
    // Check that mesh name is included
    TEST_ASSERT(strstr(content, "\"name\": \"cube_nobones\"") != NULL, 
                "Mesh should be named cube_nobones");
    
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
        
        // Basic JSON validity checks
        int brace_count = 0;
        int bracket_count = 0;
        for (char *p = content; *p; p++) {
            if (*p == '{') brace_count++;
            else if (*p == '}') brace_count--;
            else if (*p == '[') bracket_count++;
            else if (*p == ']') bracket_count--;
        }
        
        TEST_ASSERT_EQ(0, brace_count, "JSON braces should be balanced");
        TEST_ASSERT_EQ(0, bracket_count, "JSON brackets should be balanced");
        
        free(content);
    }
    
    return 1;
}

// Test 7: Verify required glTF 2.0 fields are present
static int test_gltf_required_fields(void) {
    char *content = read_file("tests/output/cube_4bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    
    // glTF 2.0 required top-level fields
    TEST_ASSERT(strstr(content, "\"asset\"") != NULL, "Must have asset");
    
    // Common fields for our use case
    TEST_ASSERT(strstr(content, "\"scenes\"") != NULL, "Should have scenes");
    TEST_ASSERT(strstr(content, "\"nodes\"") != NULL, "Should have nodes");
    TEST_ASSERT(strstr(content, "\"meshes\"") != NULL, "Should have meshes");
    TEST_ASSERT(strstr(content, "\"accessors\"") != NULL, "Should have accessors");
    TEST_ASSERT(strstr(content, "\"bufferViews\"") != NULL, "Should have bufferViews");
    TEST_ASSERT(strstr(content, "\"buffers\"") != NULL, "Should have buffers");
    
    free(content);
    return 1;
}

// Test 8: Verify animation is exported
static int test_gltf_animation_export(void) {
    char *content = read_file("tests/output/cube_4bones.gltf");
    TEST_ASSERT_NOT_NULL(content, "Should load glTF file");
    
    // Check for animation-related fields
    TEST_ASSERT(strstr(content, "\"animations\"") != NULL, "Should have animations array");
    TEST_ASSERT(strstr(content, "\"samplers\"") != NULL, "Should have animation samplers");
    TEST_ASSERT(strstr(content, "\"channels\"") != NULL, "Should have animation channels");
    
    free(content);
    return 1;
}

int main(void) {
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
    
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

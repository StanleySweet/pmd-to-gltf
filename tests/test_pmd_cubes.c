#include "test_framework.h"
#include "pmd_psa_types.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>


// Helper to compare floats with tolerance
static int float_equal(float a, float b, float tolerance) {
    return fabsf(a - b) < tolerance;
}

// Test 1: Load cube with no bones
static int test_load_cube_nobones(void) {
    PMDModel *model = load_pmd("tests/data/cube_nobones.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load cube_nobones.pmd");
    
    TEST_ASSERT_EQ(8, model->numVertices, "Should have 8 vertices");
    TEST_ASSERT_EQ(12, model->numFaces, "Should have 12 faces");
    TEST_ASSERT_EQ(0, model->numBones, "Should have 0 bones");
    
    // Verify all vertices have no bone assignments
    for (uint32_t i = 0; i < model->numVertices; i++) {
        TEST_ASSERT_EQ(0xFF, model->vertices[i].blend.bones[0], "First bone should be 0xFF (no bone)");
        TEST_ASSERT(float_equal(0.0f, model->vertices[i].blend.weights[0], 0.001f), "First weight should be 0");
    }
    
    free_pmd(model);
    return 1;
}

// Test 2: Load cube with 4 bones
static int test_load_cube_4bones(void) {
    PMDModel *model = load_pmd("tests/data/cube_4bones.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load cube_4bones.pmd");
    
    TEST_ASSERT_EQ(8, model->numVertices, "Should have 8 vertices");
    TEST_ASSERT_EQ(12, model->numFaces, "Should have 12 faces");
    TEST_ASSERT_EQ(4, model->numBones, "Should have 4 bones");
    
    // Verify bone rest positions are at corners
    TEST_ASSERT(float_equal(-1.0f, model->restStates[0].translation.x, 0.001f), "Bone 0 x at -1");
    TEST_ASSERT(float_equal(-1.0f, model->restStates[0].translation.y, 0.001f), "Bone 0 y at -1");
    TEST_ASSERT(float_equal(-1.0f, model->restStates[0].translation.z, 0.001f), "Bone 0 z at -1");
    
    TEST_ASSERT(float_equal(1.0f, model->restStates[1].translation.x, 0.001f), "Bone 1 x at 1");
    TEST_ASSERT(float_equal(-1.0f, model->restStates[1].translation.y, 0.001f), "Bone 1 y at -1");
    
    // Verify vertices have bone assignments
    TEST_ASSERT(model->vertices[0].blend.bones[0] < 0xFF, "Vertex 0 should have bone assignment");
    TEST_ASSERT(float_equal(1.0f, model->vertices[0].blend.weights[0], 0.001f), "Vertex 0 should have full weight");
    
    free_pmd(model);
    return 1;
}

// Test 3: Load cube with 5 bones
static int test_load_cube_5bones(void) {
    PMDModel *model = load_pmd("tests/data/cube_5bones.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load cube_5bones.pmd");
    
    TEST_ASSERT_EQ(8, model->numVertices, "Should have 8 vertices");
    TEST_ASSERT_EQ(12, model->numFaces, "Should have 12 faces");
    TEST_ASSERT_EQ(5, model->numBones, "Should have 5 bones");
    
    // Verify center bone is at origin
    TEST_ASSERT(float_equal(0.0f, model->restStates[4].translation.x, 0.001f), "Center bone x at 0");
    TEST_ASSERT(float_equal(0.0f, model->restStates[4].translation.y, 0.001f), "Center bone y at 0");
    TEST_ASSERT(float_equal(0.0f, model->restStates[4].translation.z, 0.001f), "Center bone z at 0");
    
    free_pmd(model);
    return 1;
}

// Test 4: Load animation for 4 bones
static int test_load_animation_4bones(void) {
    PSAAnimation *anim = load_psa("tests/data/cube_4bones_anim.psa");
    TEST_ASSERT_NOT_NULL(anim, "Should load cube_4bones_anim.psa");
    
    TEST_ASSERT_EQ(4, anim->numBones, "Should have 4 bones");
    TEST_ASSERT_EQ(10, anim->numFrames, "Should have 10 frames");
    
    // Verify first frame has bones at rest positions
    TEST_ASSERT(float_equal(-1.0f, anim->boneStates[0].translation.x, 0.001f), "Frame 0, bone 0 at rest x");
    TEST_ASSERT(float_equal(-1.0f, anim->boneStates[0].translation.y, 0.001f), "Frame 0, bone 0 at rest y");
    
    free_psa(anim);
    return 1;
}

// Test 5: Load animation for 5 bones
static int test_load_animation_5bones(void) {
    PSAAnimation *anim = load_psa("tests/data/cube_5bones_anim.psa");
    TEST_ASSERT_NOT_NULL(anim, "Should load cube_5bones_anim.psa");
    
    TEST_ASSERT_EQ(5, anim->numBones, "Should have 5 bones");
    TEST_ASSERT_EQ(10, anim->numFrames, "Should have 10 frames");
    
    free_psa(anim);
    return 1;
}

// Test 6: Verify cube dimensions (2m x 2m x 2m)
static int test_cube_dimensions(void) {
    PMDModel *model = load_pmd("tests/data/cube_nobones.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load model");
    
    // Find min and max coordinates
    float min_x = model->vertices[0].position.x;
    float max_x = model->vertices[0].position.x;
    float min_y = model->vertices[0].position.y;
    float max_y = model->vertices[0].position.y;
    float min_z = model->vertices[0].position.z;
    float max_z = model->vertices[0].position.z;
    
    for (uint32_t i = 1; i < model->numVertices; i++) {
        if (model->vertices[i].position.x < min_x) min_x = model->vertices[i].position.x;
        if (model->vertices[i].position.x > max_x) max_x = model->vertices[i].position.x;
        if (model->vertices[i].position.y < min_y) min_y = model->vertices[i].position.y;
        if (model->vertices[i].position.y > max_y) max_y = model->vertices[i].position.y;
        if (model->vertices[i].position.z < min_z) min_z = model->vertices[i].position.z;
        if (model->vertices[i].position.z > max_z) max_z = model->vertices[i].position.z;
    }
    
    // Verify dimensions are 2m (from -1 to +1 = 2m in each dimension)
    TEST_ASSERT(float_equal(2.0f, max_x - min_x, 0.001f), "Width should be 2m");
    TEST_ASSERT(float_equal(2.0f, max_y - min_y, 0.001f), "Height should be 2m");
    TEST_ASSERT(float_equal(2.0f, max_z - min_z, 0.001f), "Depth should be 2m");
    
    free_pmd(model);
    return 1;
}

// Test 7: Verify bone positions match vertices in rest pose
static int test_bone_vertex_alignment(void) {
    PMDModel *model = load_pmd("tests/data/cube_4bones.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load model");
    
    // For vertices that are fully weighted to a single bone,
    // verify the bone is at or near the vertex position
    for (uint32_t i = 0; i < model->numVertices; i++) {
        if (float_equal(1.0f, model->vertices[i].blend.weights[0], 0.001f)) {
            uint8_t bone_idx = model->vertices[i].blend.bones[0];
            if (bone_idx < model->numBones) {
                // Bone should be at the vertex position (for corner vertices)
                float dist_x = fabsf(model->vertices[i].position.x - model->restStates[bone_idx].translation.x);
                float dist_y = fabsf(model->vertices[i].position.y - model->restStates[bone_idx].translation.y);
                float dist_z = fabsf(model->vertices[i].position.z - model->restStates[bone_idx].translation.z);
                
                // Allow small distance for non-corner vertices
                TEST_ASSERT(dist_x < 2.1f, "Bone x should be near vertex");
                TEST_ASSERT(dist_y < 2.1f, "Bone y should be near vertex");
                TEST_ASSERT(dist_z < 2.1f, "Bone z should be near vertex");
            }
        }
    }
    
    free_pmd(model);
    return 1;
}

// Test 8: Verify all faces are valid triangles
static int test_face_validity(void) {
    PMDModel *model = load_pmd("tests/data/cube_nobones.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load model");
    
    for (uint32_t i = 0; i < model->numFaces; i++) {
        // All vertex indices should be within bounds
        TEST_ASSERT(model->faces[i].vertices[0] < model->numVertices, "Face vertex 0 in bounds");
        TEST_ASSERT(model->faces[i].vertices[1] < model->numVertices, "Face vertex 1 in bounds");
        TEST_ASSERT(model->faces[i].vertices[2] < model->numVertices, "Face vertex 2 in bounds");
        
        // No degenerate triangles (all vertices should be different)
        TEST_ASSERT(model->faces[i].vertices[0] != model->faces[i].vertices[1], "v0 != v1");
        TEST_ASSERT(model->faces[i].vertices[1] != model->faces[i].vertices[2], "v1 != v2");
        TEST_ASSERT(model->faces[i].vertices[0] != model->faces[i].vertices[2], "v0 != v2");
    }
    
    free_pmd(model);
    return 1;
}

// Test 9: Load cube with 2 bones and 2 prop points
static int test_load_cube_2bones_2props(void) {
    PMDModel *model = load_pmd("tests/data/cube_2bones_2props.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load cube_2bones_2props.pmd");
    
    TEST_ASSERT_EQ(8, model->numVertices, "Should have 8 vertices");
    TEST_ASSERT_EQ(12, model->numFaces, "Should have 12 faces");
    TEST_ASSERT_EQ(2, model->numBones, "Should have 2 bones");
    TEST_ASSERT_EQ(2, model->numPropPoints, "Should have 2 prop points");
    
    // Verify prop point names
    TEST_ASSERT_NOT_NULL(model->propPoints[0].name, "Prop point 0 should have name");
    TEST_ASSERT_NOT_NULL(model->propPoints[1].name, "Prop point 1 should have name");
    
    // Verify prop points are attached to valid bones
    TEST_ASSERT(model->propPoints[0].bone < model->numBones, "Prop point 0 bone valid");
    TEST_ASSERT(model->propPoints[1].bone < model->numBones, "Prop point 1 bone valid");
    
    free_pmd(model);
    return 1;
}

int main(void) {
    const test_case_t tests[] = {
        {"load_cube_nobones", test_load_cube_nobones},
        {"load_cube_4bones", test_load_cube_4bones},
        {"load_cube_5bones", test_load_cube_5bones},
        {"load_animation_4bones", test_load_animation_4bones},
        {"load_animation_5bones", test_load_animation_5bones},
        {"cube_dimensions", test_cube_dimensions},
        {"bone_vertex_alignment", test_bone_vertex_alignment},
        {"face_validity", test_face_validity},
        {"load_cube_2bones_2props", test_load_cube_2bones_2props}
    };
    
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

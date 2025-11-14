#include "test_framework.h"
#include "../pmd_psa_types.h"
#include <string.h>
#include <math.h>

// Helper to compare floats with tolerance
static int float_equal(float a, float b, float tolerance) {
    return fabs(a - b) < tolerance;
}

static int test_vector3_operations(void) {
    Vector3D v1 = {1.0f, 2.0f, 3.0f};
    
    TEST_ASSERT(float_equal(v1.x, 1.0f, 0.001f), "Vector3 x component should be correct");
    TEST_ASSERT(float_equal(v1.y, 2.0f, 0.001f), "Vector3 y component should be correct");
    TEST_ASSERT(float_equal(v1.z, 3.0f, 0.001f), "Vector3 z component should be correct");
    
    return 1;
}

static int test_quaternion_operations(void) {
    Quaternion q = {0.0f, 0.0f, 0.0f, 1.0f};
    
    TEST_ASSERT(float_equal(q.x, 0.0f, 0.001f), "Quaternion x should be correct");
    TEST_ASSERT(float_equal(q.y, 0.0f, 0.001f), "Quaternion y should be correct");
    TEST_ASSERT(float_equal(q.z, 0.0f, 0.001f), "Quaternion z should be correct");
    TEST_ASSERT(float_equal(q.w, 1.0f, 0.001f), "Quaternion w should be correct");
    
    return 1;
}

static int test_bone_state_structure(void) {
    BoneState state;
    state.translation.x = 1.0f;
    state.translation.y = 2.0f;
    state.translation.z = 3.0f;
    
    state.rotation.x = 0.0f;
    state.rotation.y = 0.0f;
    state.rotation.z = 0.0f;
    state.rotation.w = 1.0f;
    
    TEST_ASSERT(float_equal(state.translation.x, 1.0f, 0.001f), "BoneState translation x should be correct");
    TEST_ASSERT(float_equal(state.rotation.w, 1.0f, 0.001f), "BoneState rotation w should be correct");
    
    return 1;
}

static int test_vertex_structure_sizes(void) {
    // Verify structure sizes are reasonable
    TEST_ASSERT(sizeof(Vector3D) == 3 * sizeof(float), "Vector3D should be 3 floats");
    TEST_ASSERT(sizeof(Quaternion) == 4 * sizeof(float), "Quaternion should be 4 floats");
    TEST_ASSERT(sizeof(BoneState) >= sizeof(Vector3D) + sizeof(Quaternion), "BoneState should contain Vector3D and Quaternion");
    
    return 1;
}

static int test_face_structure(void) {
    Face face;
    face.vertices[0] = 0;
    face.vertices[1] = 1;
    face.vertices[2] = 2;
    
    TEST_ASSERT_EQ(0, face.vertices[0], "Face vertex 0 should be correct");
    TEST_ASSERT_EQ(1, face.vertices[1], "Face vertex 1 should be correct");
    TEST_ASSERT_EQ(2, face.vertices[2], "Face vertex 2 should be correct");
    
    return 1;
}

static int test_blend_weights_structure(void) {
    // Test basic vertex structure with correct blend field names
    Vertex vertex;
    
    // Test that we can assign values to vertex blend weights (using correct field names)
    vertex.blend.bones[0] = 5;
    vertex.blend.weights[0] = 0.8f;
    vertex.blend.bones[1] = 10;
    vertex.blend.weights[1] = 0.2f;
    vertex.blend.bones[2] = 255;  // unused bone
    vertex.blend.bones[3] = 255;  // unused bone
    vertex.blend.weights[2] = 0.0f;
    vertex.blend.weights[3] = 0.0f;
    
    TEST_ASSERT_EQ(5, vertex.blend.bones[0], "Blend bone 0 should be correct");
    TEST_ASSERT(float_equal(0.8f, vertex.blend.weights[0], 0.001f), "Blend weight 0 should be correct");
    TEST_ASSERT_EQ(10, vertex.blend.bones[1], "Blend bone 1 should be correct");
    TEST_ASSERT(float_equal(0.2f, vertex.blend.weights[1], 0.001f), "Blend weight 1 should be correct");
    TEST_ASSERT_EQ(255, vertex.blend.bones[2], "Unused blend bone should be 255");
    TEST_ASSERT_EQ(255, vertex.blend.bones[3], "Unused blend bone should be 255");
    
    return 1;
}

int main(void) {
    const test_case_t tests[] = {
        {"vector3_operations", test_vector3_operations},
        {"quaternion_operations", test_quaternion_operations},
        {"bone_state_structure", test_bone_state_structure},
        {"vertex_structure_sizes", test_vertex_structure_sizes},
        {"face_structure", test_face_structure},
        {"blend_weights_structure", test_blend_weights_structure}
    };
    
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

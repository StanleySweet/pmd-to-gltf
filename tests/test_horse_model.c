#include "test_framework.h"
#include "pmd_psa_types.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// Forward declarations of functions from pmd_parser.c
extern PMDModel* load_pmd(const char *filename);
extern void free_pmd(PMDModel *model);

// Forward declarations from psa_parser.c
extern PSAAnimation* load_psa(const char *filename);
extern void free_psa(PSAAnimation *anim);

// Helper to compare floats with tolerance
static int float_equal(float a, float b, float tolerance) {
    return fabsf(a - b) < tolerance;
}

// Test 1: Load horse PMD file
static int test_load_horse_pmd(void) {
    PMDModel *model = load_pmd("input/horse.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load horse.pmd");
    
    // Verify expected structure based on actual horse model
    TEST_ASSERT_EQ(206, model->numVertices, "Should have 206 vertices");
    TEST_ASSERT_EQ(312, model->numFaces, "Should have 312 faces");
    TEST_ASSERT_EQ(33, model->numBones, "Should have 33 bones");
    TEST_ASSERT_EQ(8, model->numPropPoints, "Should have 8 prop points");
    
    free_pmd(model);
    return 1;
}

// Test 2: Verify horse has proper bone assignments
static int test_horse_bone_weights(void) {
    PMDModel *model = load_pmd("input/horse.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load horse.pmd");
    
    // Check that vertices have valid bone assignments
    int vertices_with_bones = 0;
    for (uint32_t i = 0; i < model->numVertices; i++) {
        // Count vertices with at least one bone assignment
        if (model->vertices[i].blend.bones[0] != 0xFF) {
            vertices_with_bones++;
            
            // Verify bone indices are within valid range
            for (int j = 0; j < 4; j++) {
                if (model->vertices[i].blend.bones[j] != 0xFF) {
                    TEST_ASSERT(model->vertices[i].blend.bones[j] < model->numBones, 
                               "Bone index should be valid");
                }
            }
            
            // Verify weights sum to approximately 1.0 for weighted vertices
            float total_weight = 0.0f;
            for (int j = 0; j < 4; j++) {
                total_weight += model->vertices[i].blend.weights[j];
            }
            TEST_ASSERT(float_equal(1.0f, total_weight, 0.01f), "Weights should sum to 1.0");
        }
    }
    
    // Horse model should have skinned vertices
    TEST_ASSERT(vertices_with_bones > 0, "Should have vertices with bone assignments");
    
    free_pmd(model);
    return 1;
}

// Test 3: Verify horse rest pose bones are valid
static int test_horse_rest_pose(void) {
    PMDModel *model = load_pmd("input/horse.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load horse.pmd");
    
    // Verify all bones have valid quaternions (unit quaternions)
    for (uint32_t i = 0; i < model->numBones; i++) {
        float qx = model->restStates[i].rotation.x;
        float qy = model->restStates[i].rotation.y;
        float qz = model->restStates[i].rotation.z;
        float qw = model->restStates[i].rotation.w;
        
        float magnitude = sqrtf(qx*qx + qy*qy + qz*qz + qw*qw);
        TEST_ASSERT(float_equal(1.0f, magnitude, 0.01f), "Quaternion should be normalized");
    }
    
    free_pmd(model);
    return 1;
}

// Test 4: Load horse animation files
static int test_load_horse_animations(void) {
    PSAAnimation *anim_idle = load_psa("input/horse_idle.psa");
    TEST_ASSERT_NOT_NULL(anim_idle, "Should load horse_idle.psa");
    // Note: idle animation has 34 bones (one more than PMD), which is handled gracefully
    TEST_ASSERT_EQ(34, anim_idle->numBones, "Idle animation should have 34 bones");
    TEST_ASSERT_EQ(73, anim_idle->numFrames, "Idle animation should have 73 frames");
    free_psa(anim_idle);
    
    PSAAnimation *anim_walk = load_psa("input/horse_walk.psa");
    TEST_ASSERT_NOT_NULL(anim_walk, "Should load horse_walk.psa");
    TEST_ASSERT_EQ(33, anim_walk->numBones, "Walk animation should have 33 bones");
    TEST_ASSERT_EQ(199, anim_walk->numFrames, "Walk animation should have 199 frames");
    free_psa(anim_walk);
    
    PSAAnimation *anim_gallop = load_psa("input/horse_gallop.psa");
    TEST_ASSERT_NOT_NULL(anim_gallop, "Should load horse_gallop.psa");
    TEST_ASSERT_EQ(33, anim_gallop->numBones, "Gallop animation should have 33 bones");
    TEST_ASSERT_EQ(151, anim_gallop->numFrames, "Gallop animation should have 151 frames");
    free_psa(anim_gallop);
    
    return 1;
}

// Test 5: Verify prop points are valid
static int test_horse_prop_points(void) {
    PMDModel *model = load_pmd("input/horse.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load horse.pmd");
    
    TEST_ASSERT_EQ(8, model->numPropPoints, "Should have 8 prop points");
    
    // Verify prop points have valid parent bone indices
    for (uint32_t i = 0; i < model->numPropPoints; i++) {
        TEST_ASSERT_NOT_NULL(model->propPoints[i].name, "Prop point should have a name");
        TEST_ASSERT(model->propPoints[i].bone < model->numBones, 
                   "Prop point parent bone should be valid");
    }
    
    free_pmd(model);
    return 1;
}

// Test 6: Verify mesh bounds are reasonable
static int test_horse_mesh_bounds(void) {
    PMDModel *model = load_pmd("input/horse.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load horse.pmd");
    
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
    
    // Verify bounds match expected values (from converter output)
    TEST_ASSERT(float_equal(-0.12f, min_x, 0.01f), "Min X should be ~-0.12");
    TEST_ASSERT(float_equal(1.82f, max_x, 0.01f), "Max X should be ~1.82");
    TEST_ASSERT(float_equal(-0.59f, min_y, 0.01f), "Min Y should be ~-0.59");
    TEST_ASSERT(float_equal(0.59f, max_y, 0.01f), "Max Y should be ~0.59");
    TEST_ASSERT(float_equal(-0.68f, min_z, 0.01f), "Min Z should be ~-0.68");
    TEST_ASSERT(float_equal(0.97f, max_z, 0.01f), "Max Z should be ~0.97");
    
    free_pmd(model);
    return 1;
}

// Test 7: Verify all faces are valid
static int test_horse_face_validity(void) {
    PMDModel *model = load_pmd("input/horse.pmd");
    TEST_ASSERT_NOT_NULL(model, "Should load horse.pmd");
    
    for (uint32_t i = 0; i < model->numFaces; i++) {
        // All vertex indices should be within bounds
        TEST_ASSERT(model->faces[i].vertices[0] < model->numVertices, "Face vertex 0 in bounds");
        TEST_ASSERT(model->faces[i].vertices[1] < model->numVertices, "Face vertex 1 in bounds");
        TEST_ASSERT(model->faces[i].vertices[2] < model->numVertices, "Face vertex 2 in bounds");
        
        // No degenerate triangles
        TEST_ASSERT(model->faces[i].vertices[0] != model->faces[i].vertices[1], "v0 != v1");
        TEST_ASSERT(model->faces[i].vertices[1] != model->faces[i].vertices[2], "v1 != v2");
        TEST_ASSERT(model->faces[i].vertices[0] != model->faces[i].vertices[2], "v0 != v2");
    }
    
    free_pmd(model);
    return 1;
}

// Test 8: Verify animation bone states are valid
static int test_horse_animation_validity(void) {
    PSAAnimation *anim = load_psa("input/horse_idle.psa");
    TEST_ASSERT_NOT_NULL(anim, "Should load animation");
    
    // Check all bone states have valid quaternions
    for (uint32_t frame = 0; frame < anim->numFrames; frame++) {
        for (uint32_t bone = 0; bone < anim->numBones; bone++) {
            uint32_t idx = frame * anim->numBones + bone;
            
            float qx = anim->boneStates[idx].rotation.x;
            float qy = anim->boneStates[idx].rotation.y;
            float qz = anim->boneStates[idx].rotation.z;
            float qw = anim->boneStates[idx].rotation.w;
            
            float magnitude = sqrtf(qx*qx + qy*qy + qz*qz + qw*qw);
            TEST_ASSERT(float_equal(1.0f, magnitude, 0.01f), "Animation quaternion should be normalized");
        }
    }
    
    free_psa(anim);
    return 1;
}

int main(void) {
    const test_case_t tests[] = {
        {"load_horse_pmd", test_load_horse_pmd},
        {"horse_bone_weights", test_horse_bone_weights},
        {"horse_rest_pose", test_horse_rest_pose},
        {"load_horse_animations", test_load_horse_animations},
        {"horse_prop_points", test_horse_prop_points},
        {"horse_mesh_bounds", test_horse_mesh_bounds},
        {"horse_face_validity", test_horse_face_validity},
        {"horse_animation_validity", test_horse_animation_validity}
    };
    
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

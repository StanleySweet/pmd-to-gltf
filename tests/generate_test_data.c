#include "pmd_psa_types.h"
#include "pmd_writer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Identity quaternion (no rotation)
static Quaternion quat_identity(void) {
    Quaternion q = {0.0f, 0.0f, 0.0f, 1.0f};
    return q;
}

// Create a quaternion from axis-angle (angle in radians)
static Quaternion quat_from_axis_angle(float x, float y, float z, float angle) {
    float s = sinf(angle / 2.0f);
    float c = cosf(angle / 2.0f);
    Quaternion q;
    q.x = x * s;
    q.y = y * s;
    q.z = z * s;
    q.w = c;
    return q;
}

// Create a simple cube mesh (2m x 2m x 2m centered at origin)
// 8 vertices, 12 triangular faces
static void create_cube_mesh(PMDModel *model, int numBones) {
    model->version = 4;
    model->numTexCoords = 1;
    model->numVertices = 8;
    model->numFaces = 12;  // 6 faces * 2 triangles each
    
    model->vertices = calloc(model->numVertices, sizeof(Vertex));
    model->faces = calloc(model->numFaces, sizeof(Face));
    
    // Define 8 vertices of a 2m cube (from -1 to +1 in each dimension)
    // Vertices numbered in binary: xyz where 0=-1, 1=+1
    float positions[8][3] = {
        {-1.0f, -1.0f, -1.0f},  // 0: 000
        { 1.0f, -1.0f, -1.0f},  // 1: 100
        {-1.0f,  1.0f, -1.0f},  // 2: 010
        { 1.0f,  1.0f, -1.0f},  // 3: 110
        {-1.0f, -1.0f,  1.0f},  // 4: 001
        { 1.0f, -1.0f,  1.0f},  // 5: 101
        {-1.0f,  1.0f,  1.0f},  // 6: 011
        { 1.0f,  1.0f,  1.0f}   // 7: 111
    };
    
    // Initialize vertices
    for (int i = 0; i < 8; i++) {
        Vertex *v = &model->vertices[i];
        v->position.x = positions[i][0];
        v->position.y = positions[i][1];
        v->position.z = positions[i][2];
        
        // Simple normals pointing outward from center
        float len = sqrtf(positions[i][0]*positions[i][0] + 
                         positions[i][1]*positions[i][1] + 
                         positions[i][2]*positions[i][2]);
        v->normal.x = positions[i][0] / len;
        v->normal.y = positions[i][1] / len;
        v->normal.z = positions[i][2] / len;
        
        // Texture coordinates (simple mapping)
        v->coords = calloc(1, sizeof(TexCoord));
        v->coords[0].u = (i % 2) ? 1.0f : 0.0f;
        v->coords[0].v = ((i / 2) % 2) ? 1.0f : 0.0f;
        
        // Bone weights
        if (numBones == 0) {
            // No bones - all weights are 0xFF (no bone)
            for (int j = 0; j < 4; j++) {
                v->blend.bones[j] = 0xFF;
                v->blend.weights[j] = 0.0f;
            }
        } else if (numBones == 4) {
            // 4 bones: each corner vertex is controlled by one bone
            // Map vertex indices to bone indices:
            // v0 -> bone0, v1 -> bone1, v2 -> bone2, v3 -> bone3
            // Other vertices blend between bones
            int bone_map[8] = {0, 1, 2, 3, 0, 1, 2, 3};
            v->blend.bones[0] = bone_map[i];
            v->blend.weights[0] = 1.0f;
            v->blend.bones[1] = 0xFF;
            v->blend.weights[1] = 0.0f;
            v->blend.bones[2] = 0xFF;
            v->blend.weights[2] = 0.0f;
            v->blend.bones[3] = 0xFF;
            v->blend.weights[3] = 0.0f;
        } else if (numBones == 5) {
            // 5 bones: 4 corners + 1 center
            // Corners use their respective bones, others blend
            if (i < 4) {
                v->blend.bones[0] = i;
                v->blend.weights[0] = 1.0f;
                v->blend.bones[1] = 0xFF;
                v->blend.weights[1] = 0.0f;
            } else {
                // Vertices 4-7 blend with center bone and their corner
                int corner = i - 4;
                v->blend.bones[0] = corner;
                v->blend.weights[0] = 0.5f;
                v->blend.bones[1] = 4;  // center bone
                v->blend.weights[1] = 0.5f;
            }
            v->blend.bones[2] = 0xFF;
            v->blend.weights[2] = 0.0f;
            v->blend.bones[3] = 0xFF;
            v->blend.weights[3] = 0.0f;
        }
    }
    
    // Define 12 triangular faces (2 triangles per cube face)
    // Front face (z = -1)
    model->faces[0].vertices[0] = 0; model->faces[0].vertices[1] = 1; model->faces[0].vertices[2] = 3;
    model->faces[1].vertices[0] = 0; model->faces[1].vertices[1] = 3; model->faces[1].vertices[2] = 2;
    // Back face (z = +1)
    model->faces[2].vertices[0] = 4; model->faces[2].vertices[1] = 6; model->faces[2].vertices[2] = 7;
    model->faces[3].vertices[0] = 4; model->faces[3].vertices[1] = 7; model->faces[3].vertices[2] = 5;
    // Left face (x = -1)
    model->faces[4].vertices[0] = 0; model->faces[4].vertices[1] = 2; model->faces[4].vertices[2] = 6;
    model->faces[5].vertices[0] = 0; model->faces[5].vertices[1] = 6; model->faces[5].vertices[2] = 4;
    // Right face (x = +1)
    model->faces[6].vertices[0] = 1; model->faces[6].vertices[1] = 5; model->faces[6].vertices[2] = 7;
    model->faces[7].vertices[0] = 1; model->faces[7].vertices[1] = 7; model->faces[7].vertices[2] = 3;
    // Bottom face (y = -1)
    model->faces[8].vertices[0] = 0; model->faces[8].vertices[1] = 4; model->faces[8].vertices[2] = 5;
    model->faces[9].vertices[0] = 0; model->faces[9].vertices[1] = 5; model->faces[9].vertices[2] = 1;
    // Top face (y = +1)
    model->faces[10].vertices[0] = 2; model->faces[10].vertices[1] = 3; model->faces[10].vertices[2] = 7;
    model->faces[11].vertices[0] = 2; model->faces[11].vertices[1] = 7; model->faces[11].vertices[2] = 6;
}

// Create bone rest states for the cube
static void create_cube_bones(PMDModel *model, int numBones) {
    model->numBones = numBones;
    if (numBones == 0) {
        model->restStates = NULL;
        return;
    }
    
    model->restStates = calloc(numBones, sizeof(BoneState));
    
    if (numBones == 2) {
        // 2 bones at opposite corners (for prop point test)
        model->restStates[0].translation = (Vector3D){-1.0f, -1.0f, -1.0f};
        model->restStates[1].translation = (Vector3D){ 1.0f,  1.0f,  1.0f};
        
        for (int i = 0; i < 2; i++) {
            model->restStates[i].rotation = quat_identity();
        }
    } else if (numBones == 4) {
        // 4 bones at the bottom corners of the cube
        model->restStates[0].translation = (Vector3D){-1.0f, -1.0f, -1.0f};
        model->restStates[1].translation = (Vector3D){ 1.0f, -1.0f, -1.0f};
        model->restStates[2].translation = (Vector3D){-1.0f,  1.0f, -1.0f};
        model->restStates[3].translation = (Vector3D){ 1.0f,  1.0f, -1.0f};
        
        for (int i = 0; i < 4; i++) {
            model->restStates[i].rotation = quat_identity();
        }
    } else if (numBones == 5) {
        // 4 bones at corners + 1 at center
        model->restStates[0].translation = (Vector3D){-1.0f, -1.0f, -1.0f};
        model->restStates[1].translation = (Vector3D){ 1.0f, -1.0f, -1.0f};
        model->restStates[2].translation = (Vector3D){-1.0f,  1.0f, -1.0f};
        model->restStates[3].translation = (Vector3D){ 1.0f,  1.0f, -1.0f};
        model->restStates[4].translation = (Vector3D){ 0.0f,  0.0f,  0.0f};  // center
        
        for (int i = 0; i < 5; i++) {
            model->restStates[i].rotation = quat_identity();
        }
    }
}

// Create a simple animation with bone rotations
static PSAAnimation* create_cube_animation(int numBones, int numFrames) {
    PSAAnimation *anim = calloc(1, sizeof(PSAAnimation));
    anim->name = strdup("test_anim");
    anim->frameLength = 0.03333f;  // 30 fps
    anim->numBones = numBones;
    anim->numFrames = numFrames;
    anim->boneStates = calloc(numBones * numFrames, sizeof(BoneState));
    
    if (numBones == 4) {
        // Animate 4 corner bones with simple rotation
        for (uint32_t frame = 0; frame < (uint32_t)numFrames; frame++) {
            float t = (float)frame / (float)(numFrames - 1);
            float angle = t * 3.14159f * 2.0f;  // Full rotation over animation
            
            for (int bone = 0; bone < 4; bone++) {
                int idx = (int)(frame * (uint32_t)numBones + (uint32_t)bone);
                
                // Keep positions constant (same as rest state)
                if (bone == 0) anim->boneStates[idx].translation = (Vector3D){-1.0f, -1.0f, -1.0f};
                else if (bone == 1) anim->boneStates[idx].translation = (Vector3D){ 1.0f, -1.0f, -1.0f};
                else if (bone == 2) anim->boneStates[idx].translation = (Vector3D){-1.0f,  1.0f, -1.0f};
                else if (bone == 3) anim->boneStates[idx].translation = (Vector3D){ 1.0f,  1.0f, -1.0f};
                
                // Rotate around Z axis
                anim->boneStates[idx].rotation = quat_from_axis_angle(0.0f, 0.0f, 1.0f, angle);
            }
        }
    } else if (numBones == 5) {
        // Animate 5 bones (4 corners + center)
        for (uint32_t frame = 0; frame < (uint32_t)numFrames; frame++) {
            float t = (float)frame / (float)(numFrames - 1);
            float angle = t * 3.14159f * 2.0f;
            
            for (int bone = 0; bone < 5; bone++) {
                int idx = (int)(frame * (uint32_t)numBones + (uint32_t)bone);
                
                // Keep positions constant
                if (bone == 0) anim->boneStates[idx].translation = (Vector3D){-1.0f, -1.0f, -1.0f};
                else if (bone == 1) anim->boneStates[idx].translation = (Vector3D){ 1.0f, -1.0f, -1.0f};
                else if (bone == 2) anim->boneStates[idx].translation = (Vector3D){-1.0f,  1.0f, -1.0f};
                else if (bone == 3) anim->boneStates[idx].translation = (Vector3D){ 1.0f,  1.0f, -1.0f};
                else if (bone == 4) anim->boneStates[idx].translation = (Vector3D){ 0.0f,  0.0f,  0.0f};
                
                // Center bone rotates, corners stay fixed
                if (bone == 4) {
                    anim->boneStates[idx].rotation = quat_from_axis_angle(0.0f, 1.0f, 0.0f, angle);
                } else {
                    anim->boneStates[idx].rotation = quat_identity();
                }
            }
        }
    }
    
    return anim;
}

int main(void) {
    printf("Generating test PMD and PSA files...\n");
    
    // Create output directory if it doesn't exist
    int result = system("mkdir -p tests/data");
    (void)result;  // Intentionally ignore result
    
    // 1. Cube with no bones
    printf("Creating cube_nobones.pmd...\n");
    PMDModel model1 = {0};
    create_cube_mesh(&model1, 0);
    create_cube_bones(&model1, 0);
    model1.numPropPoints = 0;
    model1.propPoints = NULL;
    if (!write_pmd("tests/data/cube_nobones.pmd", &model1)) {
        fprintf(stderr, "Failed to write cube_nobones.pmd\n");
        return 1;
    }
    
    // 2. Cube with 4 bones
    printf("Creating cube_4bones.pmd...\n");
    PMDModel model2 = {0};
    create_cube_mesh(&model2, 4);
    create_cube_bones(&model2, 4);
    model2.numPropPoints = 0;
    model2.propPoints = NULL;
    if (!write_pmd("tests/data/cube_4bones.pmd", &model2)) {
        fprintf(stderr, "Failed to write cube_4bones.pmd\n");
        return 1;
    }
    
    // 3. Animation for 4 bones
    printf("Creating cube_4bones_anim.psa...\n");
    PSAAnimation *anim4 = create_cube_animation(4, 10);  // 10 frames
    if (!write_psa("tests/data/cube_4bones_anim.psa", anim4)) {
        fprintf(stderr, "Failed to write cube_4bones_anim.psa\n");
        return 1;
    }
    
    // 4. Cube with 5 bones (hierarchy)
    printf("Creating cube_5bones.pmd...\n");
    PMDModel model3 = {0};
    create_cube_mesh(&model3, 5);
    create_cube_bones(&model3, 5);
    model3.numPropPoints = 0;
    model3.propPoints = NULL;
    if (!write_pmd("tests/data/cube_5bones.pmd", &model3)) {
        fprintf(stderr, "Failed to write cube_5bones.pmd\n");
        return 1;
    }
    
    // 5. Animation for 5 bones
    printf("Creating cube_5bones_anim.psa...\n");
    PSAAnimation *anim5 = create_cube_animation(5, 10);  // 10 frames
    if (!write_psa("tests/data/cube_5bones_anim.psa", anim5)) {
        fprintf(stderr, "Failed to write cube_5bones_anim.psa\n");
        return 1;
    }
    
    // 6. Create skeleton XML for 5 bones with hierarchy
    printf("Creating cube_5bones.xml...\n");
    FILE *xml = fopen("tests/data/cube_5bones.xml", "w");
    if (xml) {
        fprintf(xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(xml, "<skeleton target=\"cube_5bones\">\n");
        fprintf(xml, "  <bone name=\"bone_center\">\n");
        fprintf(xml, "    <bone name=\"bone_corner_0\"/>\n");
        fprintf(xml, "    <bone name=\"bone_corner_1\"/>\n");
        fprintf(xml, "    <bone name=\"bone_corner_2\"/>\n");
        fprintf(xml, "    <bone name=\"bone_corner_3\"/>\n");
        fprintf(xml, "  </bone>\n");
        fprintf(xml, "</skeleton>\n");
        fclose(xml);
    }
    
    // 7. Cube with 2 bones and 2 prop points (to test prop point handling)
    printf("Creating cube_2bones_2props.pmd...\n");
    PMDModel model4 = {0};
    create_cube_mesh(&model4, 2);
    create_cube_bones(&model4, 2);
    
    // Add 2 prop points
    model4.numPropPoints = 2;
    model4.propPoints = calloc(2, sizeof(PropPoint));
    
    // Prop point 0: weapon attachment at bone 0
    model4.propPoints[0].name = strdup("prop_weapon");
    model4.propPoints[0].translation = (Vector3D){0.5f, 0.0f, 0.0f};
    model4.propPoints[0].rotation = quat_identity();
    model4.propPoints[0].bone = 0;
    
    // Prop point 1: shield attachment at bone 1
    model4.propPoints[1].name = strdup("prop_shield");
    model4.propPoints[1].translation = (Vector3D){-0.5f, 0.0f, 0.0f};
    model4.propPoints[1].rotation = quat_identity();
    model4.propPoints[1].bone = 1;
    
    if (!write_pmd("tests/data/cube_2bones_2props.pmd", &model4)) {
        fprintf(stderr, "Failed to write cube_2bones_2props.pmd\n");
        return 1;
    }
    
    printf("Test files generated successfully!\n");
    printf("\nGenerated files:\n");
    printf("  - tests/data/cube_nobones.pmd\n");
    printf("  - tests/data/cube_4bones.pmd\n");
    printf("  - tests/data/cube_4bones_anim.psa\n");
    printf("  - tests/data/cube_5bones.pmd\n");
    printf("  - tests/data/cube_5bones_anim.psa\n");
    printf("  - tests/data/cube_5bones.xml\n");
    printf("  - tests/data/cube_2bones_2props.pmd\n");
    
    return 0;
}

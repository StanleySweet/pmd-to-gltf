#include "pmd_psa_types.h"
#include "skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static char* create_data_uri(const void *data, size_t size) {
    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t encoded_size = 4 * ((size + 2) / 3);
    char *encoded = calloc(encoded_size + 100, 1);
    strcpy(encoded, "data:application/octet-stream;base64,");

    const unsigned char *bytes = (const unsigned char *)data;
    char *out = encoded + strlen(encoded);

    for (size_t i = 0; i < size; ) {
        uint32_t octet_a = i < size ? bytes[i++] : 0;
        uint32_t octet_b = i < size ? bytes[i++] : 0;
        uint32_t octet_c = i < size ? bytes[i++] : 0;
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        *out++ = base64_chars[(triple >> 18) & 0x3F];
        *out++ = base64_chars[(triple >> 12) & 0x3F];
        *out++ = (i > size + 1) ? '=' : base64_chars[(triple >> 6) & 0x3F];
        *out++ = (i > size) ? '=' : base64_chars[triple & 0x3F];
    }
    *out = '\0';
    return encoded;
}

static int find_prop_parent_bone(const char *prop_name, SkeletonDef *skel) {
    if (!skel || !prop_name) return -1;

    const char *bone_part = strstr(prop_name, "prop_");
    if (!bone_part) bone_part = strstr(prop_name, "prop-");
    if (!bone_part) bone_part = strstr(prop_name, "prop.");
    if (bone_part) bone_part += 5;
    else bone_part = prop_name;

    for (int i = 0; i < skel->bone_count; i++) {
        if (strstr(skel->bones[i].name, bone_part)) {
            return i;
        }
    }
    return -1;
}

// Quaternion inverse
static Quaternion quat_inverse(Quaternion q) {
    float len2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    return (Quaternion){-q.x/len2, -q.y/len2, -q.z/len2, q.w/len2};
}

// Quaternion multiply
static Quaternion quat_mul(Quaternion a, Quaternion b) {
    return (Quaternion){
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    };
}

// Rotate vector by quaternion
static Vector3D quat_rotate(Quaternion q, Vector3D v) {
    Vector3D qv = {q.x, q.y, q.z};
    float qw = q.w;

    Vector3D cross1 = {qv.y*v.z - qv.z*v.y, qv.z*v.x - qv.x*v.z, qv.x*v.y - qv.y*v.x};
    Vector3D cross2 = {qv.y*cross1.z - qv.z*cross1.y, qv.z*cross1.x - qv.x*cross1.z, qv.x*cross1.y - qv.y*cross1.x};

    return (Vector3D){
        v.x + 2.0f * (qw * cross1.x + cross2.x),
        v.y + 2.0f * (qw * cross1.y + cross2.y),
        v.z + 2.0f * (qw * cross1.z + cross2.z)
    };
}

// Convert world space transform to local space relative to parent
static void compute_local_transform(BoneState *local, const BoneState *world, const BoneState *parent_world) {
    // local_rotation = inverse(parent_rotation) * world_rotation
    Quaternion parent_inv = quat_inverse(parent_world->rotation);
    local->rotation = quat_mul(parent_inv, world->rotation);

    // local_translation = inverse(parent_rotation) * (world_translation - parent_translation)
    Vector3D diff = {
        world->translation.x - parent_world->translation.x,
        world->translation.y - parent_world->translation.y,
        world->translation.z - parent_world->translation.z
    };
    local->translation = quat_rotate(parent_inv, diff);
}void export_gltf(const char *output_file, PMDModel *model, PSAAnimation *anim, SkeletonDef *skel, const char *mesh_name) {
    FILE *f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to create output file\n");
        return;
    }

    uint32_t skel_bones = skel ? skel->bone_count : model->numBones;
    uint32_t total_bones = model->numBones + model->numPropPoints;

    // Create bone index mapping: exclude only root (index 0) from skinning
    // Skinnable bones are skeleton bones 1 to numBones-1 (props are not skinnable)
    uint32_t skinnable_bones = model->numBones > 1 ? model->numBones - 1 : model->numBones;
    int *bone_to_joint = calloc(model->numBones, sizeof(int));
    for (uint32_t i = 0; i < model->numBones; i++) {
        if (skel && i == 0) {
            bone_to_joint[i] = -1; // Root bone - not skinnable
        } else {
            bone_to_joint[i] = i - 1; // Subtract 1 to skip root
        }
    }

    size_t positions_size = model->numVertices * 3 * sizeof(float);
    size_t normals_size = model->numVertices * 3 * sizeof(float);
    size_t texcoords_size = model->numVertices * 2 * sizeof(float);
    size_t indices_size = model->numFaces * 3 * sizeof(uint16_t);
    size_t joints_size = model->numVertices * 4 * sizeof(uint16_t);
    size_t weights_size = model->numVertices * 4 * sizeof(float);

    float *positions = calloc(model->numVertices * 3, sizeof(float));
    float *normals = calloc(model->numVertices * 3, sizeof(float));
    float *texcoords = calloc(model->numVertices * 2, sizeof(float));
    uint16_t *indices = calloc(model->numFaces * 3, sizeof(uint16_t));
    uint16_t *joints = calloc(model->numVertices * 4, sizeof(uint16_t));
    float *weights = calloc(model->numVertices * 4, sizeof(float));

    // For glTF with hierarchy: bones use local transforms, but vertices are in world space
    // PMD stores bones in world space, so we keep them that way (no hierarchy conversion)
    // and use identity inverse bind matrices since vertices match the rest pose

    // Validation: Track min/max vertex positions
    Vector3D min_pos = {1e10f, 1e10f, 1e10f};
    Vector3D max_pos = {-1e10f, -1e10f, -1e10f};

    for (uint32_t i = 0; i < model->numVertices; i++) {
        positions[i*3+0] = model->vertices[i].position.x;
        positions[i*3+1] = model->vertices[i].position.y;
        positions[i*3+2] = model->vertices[i].position.z;

        // Track bounds
        if (model->vertices[i].position.x < min_pos.x) min_pos.x = model->vertices[i].position.x;
        if (model->vertices[i].position.y < min_pos.y) min_pos.y = model->vertices[i].position.y;
        if (model->vertices[i].position.z < min_pos.z) min_pos.z = model->vertices[i].position.z;
        if (model->vertices[i].position.x > max_pos.x) max_pos.x = model->vertices[i].position.x;
        if (model->vertices[i].position.y > max_pos.y) max_pos.y = model->vertices[i].position.y;
        if (model->vertices[i].position.z > max_pos.z) max_pos.z = model->vertices[i].position.z;

        normals[i*3+0] = model->vertices[i].normal.x;
        normals[i*3+1] = model->vertices[i].normal.y;
        normals[i*3+2] = model->vertices[i].normal.z;

        texcoords[i*2+0] = model->vertices[i].coords[0].u;
        texcoords[i*2+1] = model->vertices[i].coords[0].v;

        float total_weight = 0.0f;
        int valid_count = 0;
        for (int j = 0; j < 4; j++) {
            uint8_t bone_idx = model->vertices[i].blend.bones[j];
            if (bone_idx != 0xFF && bone_idx < model->numBones) {
                // Exclude root (0) and props (>= skel_bones) from vertex weighting
                if (bone_idx == 0 || (skel && bone_idx >= skel_bones)) {
                    continue; // Skip root and props - don't use for deformation
                }

                int joint_idx = bone_to_joint[bone_idx];
                if (joint_idx >= 0) {
                    joints[i*4+valid_count] = joint_idx;
                    weights[i*4+valid_count] = model->vertices[i].blend.weights[j];
                    total_weight += model->vertices[i].blend.weights[j];
                    valid_count++;
                }
            }
        }

        // If no valid joints after filtering, assign to pelvis (bone 1, joint 0)
        if (valid_count == 0) {
            joints[i*4+0] = 0; // Pelvis joint
            weights[i*4+0] = 1.0f;
            valid_count = 1;
            total_weight = 1.0f;
        }

        for (int j = valid_count; j < 4; j++) {
            joints[i*4+j] = 0;
            weights[i*4+j] = 0.0f;
        }

        if (total_weight > 0.0f && total_weight != 1.0f) {
            for (int j = 0; j < valid_count; j++) {
                weights[i*4+j] /= total_weight;
            }
        }
    }

    printf("  Mesh bounds: (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)\n",
           min_pos.x, min_pos.y, min_pos.z, max_pos.x, max_pos.y, max_pos.z);

    for (uint32_t i = 0; i < model->numFaces; i++) {
        indices[i*3+0] = model->faces[i].vertices[0];
        indices[i*3+1] = model->faces[i].vertices[1];
        indices[i*3+2] = model->faces[i].vertices[2];
    }

    // Compute inverse bind matrices from rest pose
    // Vertices are in world space matching rest pose, so use identity matrices
    // IBMs for skeleton bones + prop points (prop points also get identity since they don't deform)
    uint32_t total_ibm_count = skinnable_bones + model->numPropPoints;
    size_t ibm_size = total_ibm_count * 16 * sizeof(float);
    float *ibm = calloc(total_ibm_count * 16, sizeof(float));
    for (uint32_t i = 0; i < total_ibm_count; i++) {
        uint32_t idx = i * 16;
        // Identity matrix in column-major order
        ibm[idx + 0] = 1.0f; ibm[idx + 4] = 0.0f; ibm[idx + 8]  = 0.0f; ibm[idx + 12] = 0.0f;
        ibm[idx + 1] = 0.0f; ibm[idx + 5] = 1.0f; ibm[idx + 9]  = 0.0f; ibm[idx + 13] = 0.0f;
        ibm[idx + 2] = 0.0f; ibm[idx + 6] = 0.0f; ibm[idx + 10] = 1.0f; ibm[idx + 14] = 0.0f;
        ibm[idx + 3] = 0.0f; ibm[idx + 7] = 0.0f; ibm[idx + 11] = 0.0f; ibm[idx + 15] = 1.0f;
    }

    // Prepare animation data if available
    float *anim_times = NULL;
    float **anim_translations = NULL;
    float **anim_rotations = NULL;
    char **anim_trans_uris = NULL;
    char **anim_rot_uris = NULL;
    char *anim_times_uri = NULL;
    uint32_t anim_bones = 0;
    size_t times_size = 0;
    size_t trans_size = 0;
    size_t rot_size = 0;

    if (anim && anim->numFrames > 0) {
        anim_bones = anim->numBones < model->numBones ? anim->numBones : model->numBones;

        // Time samples (30 fps)
        anim_times = calloc(anim->numFrames, sizeof(float));
        for (uint32_t i = 0; i < anim->numFrames; i++) {
            anim_times[i] = (float)i / 30.0f;
        }
        times_size = anim->numFrames * sizeof(float);
        anim_times_uri = create_data_uri(anim_times, times_size);

        // Per-bone translation and rotation arrays
        anim_translations = calloc(anim_bones, sizeof(float*));
        anim_rotations = calloc(anim_bones, sizeof(float*));
        anim_trans_uris = calloc(anim_bones, sizeof(char*));
        anim_rot_uris = calloc(anim_bones, sizeof(char*));
        trans_size = anim->numFrames * 3 * sizeof(float);
        rot_size = anim->numFrames * 4 * sizeof(float);

        for (uint32_t b = 0; b < anim_bones; b++) {
            anim_translations[b] = calloc(anim->numFrames * 3, sizeof(float));
            anim_rotations[b] = calloc(anim->numFrames * 4, sizeof(float));

            for (uint32_t f = 0; f < anim->numFrames; f++) {
                BoneState *state = &anim->boneStates[f * anim->numBones + b];

                // Convert world space to local space if bone has parent
                BoneState local_state = *state;
                if (skel && b < skel->bone_count && skel->bones[b].parent_index != -1) {
                    int parent_idx = skel->bones[b].parent_index;
                    BoneState *parent_state = &anim->boneStates[f * anim->numBones + parent_idx];
                    compute_local_transform(&local_state, state, parent_state);
                }

                anim_translations[b][f*3 + 0] = local_state.translation.x;
                anim_translations[b][f*3 + 1] = local_state.translation.y;
                anim_translations[b][f*3 + 2] = local_state.translation.z;

                anim_rotations[b][f*4 + 0] = local_state.rotation.x;
                anim_rotations[b][f*4 + 1] = local_state.rotation.y;
                anim_rotations[b][f*4 + 2] = local_state.rotation.z;
                anim_rotations[b][f*4 + 3] = local_state.rotation.w;
            }

            anim_trans_uris[b] = create_data_uri(anim_translations[b], trans_size);
            anim_rot_uris[b] = create_data_uri(anim_rotations[b], rot_size);
        }
    }

    char *pos_uri = create_data_uri(positions, positions_size);
    char *norm_uri = create_data_uri(normals, normals_size);
    char *tex_uri = create_data_uri(texcoords, texcoords_size);
    char *idx_uri = create_data_uri(indices, indices_size);
    char *joints_uri = create_data_uri(joints, joints_size);
    char *weights_uri = create_data_uri(weights, weights_size);
    char *ibm_uri = create_data_uri(ibm, ibm_size);

    fprintf(f, "{\n");
    fprintf(f, "  \"asset\": {\"version\": \"2.0\", \"generator\": \"PMD-PSA-Converter\"},\n");
    fprintf(f, "  \"scene\": 0,\n");
    fprintf(f, "  \"scenes\": [{\"nodes\": [0]}],\n");

    // Nodes: armature root + mesh + bones
    fprintf(f, "  \"nodes\": [\n");

    // Node 0: Armature root (named after skeleton)
    const char *armature_name = skel ? "Horse Biped" : "Armature";
    fprintf(f, "    {\"name\": \"%s\", \"children\": [1", armature_name);

    // Add root bones as children (hierarchy mode)
    if (skel) {
        for (int i = 0; i < skel->bone_count; i++) {
            if (skel->bones[i].parent_index == -1) {
                fprintf(f, ", %u", i + 2);
            }
        }
        // Add orphan prop points (parent bone is 0xFF or invalid)
        for (uint32_t i = 0; i < model->numPropPoints; i++) {
            uint8_t parent_bone = model->propPoints[i].bone;
            if (parent_bone == 0xFF || parent_bone >= model->numBones) {
                fprintf(f, ", %u", model->numBones + i + 2);
            }
        }
    } else {
        // No skeleton: flat hierarchy
        for (uint32_t i = 0; i < model->numBones; i++) {
            fprintf(f, ", %u", i + 2);
        }
    }
    fprintf(f, "]},\n");

    // Node 1: Mesh (child of armature, uses skin)
    fprintf(f, "    {\"mesh\": 0, \"skin\": 0}");

    // Bone nodes (start at index 2)
    for (uint32_t i = 0; i < total_bones; i++) {
        fprintf(f, ",\n    {");

        if (i < model->numBones) {
            // Regular skeleton bones
            if (skel && i < skel->bone_count) {
                fprintf(f, "\"name\": \"%s\"", skel->bones[i].name);
            } else {
                fprintf(f, "\"name\": \"bone_%u\"", i);
            }
        } else {
            // Prop point bones
            uint32_t prop_idx = i - model->numBones;
            fprintf(f, "\"name\": \"prop-%s\"", model->propPoints[prop_idx].name);
        }

        // Compute transform (local if has parent, world if root)
        BoneState transform;
        if (i < model->numBones) {
            // Regular skeleton bone
            transform = model->restStates[i];
            if (skel && i < skel->bone_count && skel->bones[i].parent_index != -1) {
                int parent_idx = skel->bones[i].parent_index;
                compute_local_transform(&transform, &model->restStates[i], &model->restStates[parent_idx]);
            }
        } else {
            // Prop point - use local offset from propPoints
            uint32_t prop_idx = i - model->numBones;
            transform.translation = model->propPoints[prop_idx].translation;
            transform.rotation = model->propPoints[prop_idx].rotation;
        }

        fprintf(f, ", \"translation\": [%f, %f, %f], \"rotation\": [%f, %f, %f, %f]",
                transform.translation.x,
                transform.translation.y,
                transform.translation.z,
                transform.rotation.x,
                transform.rotation.y,
                transform.rotation.z,
                transform.rotation.w);

        // Add children
        if (i < model->numBones) {
            int has_children = 0;
            // Skeleton children
            if (skel) {
                for (int j = 0; j < skel->bone_count; j++) {
                    if (skel->bones[j].parent_index == (int)i) {
                        if (!has_children) {
                            fprintf(f, ", \"children\": [");
                            has_children = 1;
                        } else {
                            fprintf(f, ", ");
                        }
                        fprintf(f, "%u", j + 2);
                    }
                }
            }
            // Prop point children
            for (uint32_t j = 0; j < model->numPropPoints; j++) {
                uint8_t parent_bone = model->propPoints[j].bone;
                if (parent_bone == i) {
                    if (!has_children) {
                        fprintf(f, ", \"children\": [");
                        has_children = 1;
                    } else {
                        fprintf(f, ", ");
                    }
                    fprintf(f, "%u", model->numBones + j + 2);
                }
            }
            if (has_children) fprintf(f, "]");
        }

        fprintf(f, "}");
    }
    fprintf(f, "\n  ],\n");

    fprintf(f, "  \"meshes\": [{\"name\": \"%s\", \"primitives\": [{\"attributes\": {\"POSITION\": 0, \"NORMAL\": 1, \"TEXCOORD_0\": 2, \"JOINTS_0\": 3, \"WEIGHTS_0\": 4}, \"indices\": 5}]}],\n", mesh_name);

    fprintf(f, "  \"accessors\": [\n");
    fprintf(f, "    {\"bufferView\": 0, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC3\"},\n", model->numVertices);
    fprintf(f, "    {\"bufferView\": 1, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC3\"},\n", model->numVertices);
    fprintf(f, "    {\"bufferView\": 2, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC2\"},\n", model->numVertices);
    fprintf(f, "    {\"bufferView\": 3, \"componentType\": 5123, \"count\": %u, \"type\": \"VEC4\"},\n", model->numVertices);
    fprintf(f, "    {\"bufferView\": 4, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC4\"},\n", model->numVertices);
    fprintf(f, "    {\"bufferView\": 5, \"componentType\": 5123, \"count\": %u, \"type\": \"SCALAR\"},\n", model->numFaces * 3);
    fprintf(f, "    {\"bufferView\": 6, \"componentType\": 5126, \"count\": %u, \"type\": \"MAT4\"}",  skinnable_bones + model->numPropPoints);

    // Animation accessors
    if (anim && anim->numFrames > 0) {
        fprintf(f, ",\n");
        // Accessor 7: Time samples (shared by all bones)
        fprintf(f, "    {\"bufferView\": 7, \"componentType\": 5126, \"count\": %u, \"type\": \"SCALAR\", \"min\": [0.0], \"max\": [%f]}",
                anim->numFrames, (float)(anim->numFrames - 1) / 30.0f);

        // Accessors 8+: Per-bone translations and rotations
        for (uint32_t b = 0; b < anim_bones; b++) {
            fprintf(f, ",\n");
            fprintf(f, "    {\"bufferView\": %u, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC3\"}",
                    8 + b * 2, anim->numFrames);
            fprintf(f, ",\n");
            fprintf(f, "    {\"bufferView\": %u, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC4\"}",
                    8 + b * 2 + 1, anim->numFrames);
        }
    }

    fprintf(f, "\n  ],\n");

    fprintf(f, "  \"bufferViews\": [\n");
    fprintf(f, "    {\"buffer\": 0, \"byteLength\": %zu},\n", positions_size);
    fprintf(f, "    {\"buffer\": 1, \"byteLength\": %zu},\n", normals_size);
    fprintf(f, "    {\"buffer\": 2, \"byteLength\": %zu},\n", texcoords_size);
    fprintf(f, "    {\"buffer\": 3, \"byteLength\": %zu},\n", joints_size);
    fprintf(f, "    {\"buffer\": 4, \"byteLength\": %zu},\n", weights_size);
    fprintf(f, "    {\"buffer\": 5, \"byteLength\": %zu},\n", indices_size);
    fprintf(f, "    {\"buffer\": 6, \"byteLength\": %zu}", ibm_size);

    // Animation buffer views
    if (anim && anim->numFrames > 0) {
        fprintf(f, ",\n");
        // Buffer view 7: Time samples
        fprintf(f, "    {\"buffer\": 7, \"byteLength\": %zu}", times_size);

        // Buffer views 8+: Per-bone translations and rotations
        for (uint32_t b = 0; b < anim_bones; b++) {
            fprintf(f, ",\n");
            fprintf(f, "    {\"buffer\": %u, \"byteLength\": %zu}", 8 + b * 2, trans_size);
            fprintf(f, ",\n");
            fprintf(f, "    {\"buffer\": %u, \"byteLength\": %zu}", 8 + b * 2 + 1, rot_size);
        }
    }

    fprintf(f, "\n  ],\n");

    fprintf(f, "  \"buffers\": [\n");
    fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"},\n", positions_size, pos_uri);
    fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"},\n", normals_size, norm_uri);
    fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"},\n", texcoords_size, tex_uri);
    fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"},\n", joints_size, joints_uri);
    fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"},\n", weights_size, weights_uri);
    fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"},\n", indices_size, idx_uri);
    fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"}", ibm_size, ibm_uri);

    // Animation buffers
    if (anim && anim->numFrames > 0) {
        fprintf(f, ",\n");
        // Buffer 7: Time samples
        fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"}", times_size, anim_times_uri);

        // Buffers 8+: Per-bone translations and rotations
        for (uint32_t b = 0; b < anim_bones; b++) {
            fprintf(f, ",\n");
            fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"}", trans_size, anim_trans_uris[b]);
            fprintf(f, ",\n");
            fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"}", rot_size, anim_rot_uris[b]);
        }
    }

    fprintf(f, "\n  ],\n");

    fprintf(f, "  \"skins\": [{\"skeleton\": 0, \"inverseBindMatrices\": 6, \"joints\": [");
    // Include skeleton bones (except root) + all prop points
    // Skeleton bones: 1 through numBones-1
    for (uint32_t i = 0; i < skinnable_bones; i++) {
        // Node index = bone index + 2, but we're skipping root (bone 0), so it's (i+1)+2 = i+3
        fprintf(f, "%u, ", i + 3);
    }
    // Prop point bones: numBones through numBones+numPropPoints-1
    for (uint32_t i = 0; i < model->numPropPoints; i++) {
        uint32_t node_index = model->numBones + i + 2;
        fprintf(f, "%u%s", node_index, (i < model->numPropPoints - 1) ? ", " : "");
    }
    fprintf(f, "]}]");

    // Add animation if available
    if (anim && anim->numFrames > 0) {
        fprintf(f, ",\n");
        fprintf(f, "  \"animations\": [{\n");
        fprintf(f, "    \"name\": \"Animation\",\n");
        fprintf(f, "    \"samplers\": [\n");

        // Create samplers: one for translation, one for rotation per bone
        for (uint32_t b = 0; b < anim_bones; b++) {
            uint32_t time_accessor = 7; // Shared time accessor
            uint32_t trans_accessor = 8 + b * 2;
            uint32_t rot_accessor = 8 + b * 2 + 1;

            // Translation sampler
            fprintf(f, "      {\"input\": %u, \"output\": %u, \"interpolation\": \"LINEAR\"}",
                    time_accessor, trans_accessor);

            // Rotation sampler
            fprintf(f, ",\n      {\"input\": %u, \"output\": %u, \"interpolation\": \"LINEAR\"}",
                    time_accessor, rot_accessor);

            if (b < anim_bones - 1) fprintf(f, ",\n");
        }

        fprintf(f, "\n    ],\n");
        fprintf(f, "    \"channels\": [\n");

        // Create channels connecting samplers to bone nodes
        for (uint32_t b = 0; b < anim_bones; b++) {
            uint32_t trans_sampler = b * 2;
            uint32_t rot_sampler = b * 2 + 1;
            uint32_t node = b + 2; // Bone nodes start at index 2

            // Translation channel
            fprintf(f, "      {\"sampler\": %u, \"target\": {\"node\": %u, \"path\": \"translation\"}}",
                    trans_sampler, node);

            // Rotation channel
            fprintf(f, ",\n      {\"sampler\": %u, \"target\": {\"node\": %u, \"path\": \"rotation\"}}",
                    rot_sampler, node);

            if (b < anim_bones - 1) fprintf(f, ",\n");
        }

        fprintf(f, "\n    ]\n");
        fprintf(f, "  }]");
    }

    fprintf(f, "\n}\n");

    fclose(f);

    free(positions);
    free(normals);
    free(texcoords);
    free(indices);
    free(joints);
    free(weights);
    free(ibm);
    free(bone_to_joint);
    free(pos_uri);
    free(norm_uri);
    free(tex_uri);
    free(idx_uri);
    free(joints_uri);
    free(weights_uri);
    free(ibm_uri);

    // Free animation data
    if (anim && anim->numFrames > 0) {
        free(anim_times);
        free(anim_times_uri);
        for (uint32_t b = 0; b < anim_bones; b++) {
            free(anim_translations[b]);
            free(anim_rotations[b]);
            free(anim_trans_uris[b]);
            free(anim_rot_uris[b]);
        }
        free(anim_translations);
        free(anim_rotations);
        free(anim_trans_uris);
        free(anim_rot_uris);
    }
}

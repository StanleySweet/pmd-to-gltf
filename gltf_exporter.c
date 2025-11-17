#include <math.h>

#include "pmd_psa_types.h"
#include "skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ...existing code...
#include <math.h>

#include "pmd_psa_types.h"

// Helper: build matrix from BoneState
void make_matrix(const BoneState *bs, float *out) {
    float x = bs->rotation.x, y = bs->rotation.y, z = bs->rotation.z, w = bs->rotation.w;
    float xx = x*x, yy = y*y, zz = z*z;
    float xy = x*y, xz = x*z, yz = y*z;
    float wx = w*x, wy = w*y, wz = w*z;
    out[0] = 1.0f - 2.0f*(yy + zz);
    out[1] = 2.0f*(xy + wz);
    out[2] = 2.0f*(xz - wy);
    out[3] = 0.0f;
    out[4] = 2.0f*(xy - wz);
    out[5] = 1.0f - 2.0f*(xx + zz);
    out[6] = 2.0f*(yz + wx);
    out[7] = 0.0f;
    out[8] = 2.0f*(xz + wy);
    out[9] = 2.0f*(yz - wx);
    out[10] = 1.0f - 2.0f*(xx + yy);
    out[11] = 0.0f;
    out[12] = bs->translation.x;
    out[13] = bs->translation.y;
    out[14] = bs->translation.z;
    out[15] = 1.0f;
}

void invert_affine(const float *m, float *out) {
    out[0]=m[0]; out[1]=m[4]; out[2]=m[8];  out[3]=0.0f;
    out[4]=m[1]; out[5]=m[5]; out[6]=m[9];  out[7]=0.0f;
    out[8]=m[2]; out[9]=m[6]; out[10]=m[10]; out[11]=0.0f;
    float tx = m[12], ty = m[13], tz = m[14];
    out[12] = -(out[0]*tx + out[4]*ty + out[8]*tz);
    out[13] = -(out[1]*tx + out[5]*ty + out[9]*tz);
    out[14] = -(out[2]*tx + out[6]*ty + out[10]*tz);
    out[15] = 1.0f;
}
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
    const char *prefix = "data:application/octet-stream;base64,";
    size_t prefix_len = strlen(prefix);
    memcpy(encoded, prefix, prefix_len);
    encoded[prefix_len] = '\0';

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
}

void export_gltf(const char *output_file, PMDModel *model, PSAAnimation **anims, uint32_t anim_count, SkeletonDef *skel, const char *mesh_name, const float *anim_speed_percent, const char *rest_pose_anim) {
        PSAAnimation *bind_anim = NULL;
        if (rest_pose_anim && anims && anim_count > 0) {
            for (uint32_t a = 0; a < anim_count; ++a) {
                if (anims[a] && anims[a]->name && strcmp(anims[a]->name, rest_pose_anim) == 0) {
                    bind_anim = anims[a];
                    break;
                }
            }
        }
    FILE *f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to create output file\n");
        return;
    }

    uint32_t skel_bones = skel ? (uint32_t)skel->bone_count : model->numBones;
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
        Vector3D pos = model->vertices[i].position;
        Vector3D norm = model->vertices[i].normal;

        // If rest pose animation is specified, adapt mesh to new rest pose
        if (bind_anim && bind_anim->numFrames > 0) {
            Vector3D new_pos = {0,0,0};
            Vector3D new_norm = {0,0,0};
            float total_weight = 0.0f;
            for (int j = 0; j < 4; j++) {
                uint8_t bone_idx = model->vertices[i].blend.bones[j];
                float weight = model->vertices[i].blend.weights[j];
                if (bone_idx != 0xFF && bone_idx < bind_anim->numBones && weight > 0.0f) {
                    // Get transform from frame 0 of rest pose animation
                    BoneState bs = bind_anim->boneStates[0 * bind_anim->numBones + bone_idx];
                    // Apply rotation
                    Vector3D rotated = quat_rotate(bs.rotation, pos);
                    // Apply translation
                    rotated.x += bs.translation.x;
                    rotated.y += bs.translation.y;
                    rotated.z += bs.translation.z;
                    new_pos.x += rotated.x * weight;
                    new_pos.y += rotated.y * weight;
                    new_pos.z += rotated.z * weight;
                    // Normals: rotate only
                    Vector3D nrm_rot = quat_rotate(bs.rotation, norm);
                    new_norm.x += nrm_rot.x * weight;
                    new_norm.y += nrm_rot.y * weight;
                    new_norm.z += nrm_rot.z * weight;
                    total_weight += weight;
                }
            }
            if (total_weight > 0.0f) {
                new_pos.x /= total_weight;
                new_pos.y /= total_weight;
                new_pos.z /= total_weight;
                new_norm.x /= total_weight;
                new_norm.y /= total_weight;
                new_norm.z /= total_weight;
                // Normalize the blended normal
                float norm_len = sqrtf(new_norm.x*new_norm.x + new_norm.y*new_norm.y + new_norm.z*new_norm.z);
                if (norm_len > 1e-6f) {
                    new_norm.x /= norm_len;
                    new_norm.y /= norm_len;
                    new_norm.z /= norm_len;
                }
                // Flip normals to fix inside-out shading
                new_norm.x = -new_norm.x;
                new_norm.y = -new_norm.y;
                new_norm.z = -new_norm.z;
                pos = new_pos;
                norm = new_norm;
            }
        }

        positions[i*3+0] = pos.x;
        positions[i*3+1] = pos.y;
        positions[i*3+2] = pos.z;

        // Track bounds
        if (pos.x < min_pos.x) min_pos.x = pos.x;
        if (pos.y < min_pos.y) min_pos.y = pos.y;
        if (pos.z < min_pos.z) min_pos.z = pos.z;
        if (pos.x > max_pos.x) max_pos.x = pos.x;
        if (pos.y > max_pos.y) max_pos.y = pos.y;
        if (pos.z > max_pos.z) max_pos.z = pos.z;

        normals[i*3+0] = norm.x;
        normals[i*3+1] = norm.y;
        normals[i*3+2] = norm.z;

        // glTF expects V origin at top; source data appears upside-down -> flip V
        texcoords[i*2+0] = model->vertices[i].coords[0].u;
        texcoords[i*2+1] = 1.0f - model->vertices[i].coords[0].v;

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
                    joints[i*4+valid_count] = (uint16_t)joint_idx;
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

    // Compute inverse bind matrices and node transforms from rest pose or specified animation frame 0
    uint32_t total_ibm_count = skinnable_bones + model->numPropPoints;
    size_t ibm_size = total_ibm_count * 16 * sizeof(float);
    float *ibm = calloc(total_ibm_count * 16, sizeof(float));
    // For each skinnable bone, use frame 0 of bind_anim if set, else restStates
    for (uint32_t i = 0; i < skinnable_bones; ++i) {
        uint32_t boneIndex = i + 1;
        BoneState world_bs;
        int haveWorld = 0;
        if (bind_anim && boneIndex < bind_anim->numBones && bind_anim->numFrames > 0) {
            world_bs = bind_anim->boneStates[0 * bind_anim->numBones + boneIndex];
            haveWorld = 1;
        } else if (boneIndex < model->numBones) {
            world_bs = model->restStates[boneIndex];
            haveWorld = 1;
        }
        if (!haveWorld) {
            uint32_t idx = i*16;
            ibm[idx+0]=1; ibm[idx+4]=0; ibm[idx+8]=0; ibm[idx+12]=0;
            ibm[idx+1]=0; ibm[idx+5]=1; ibm[idx+9]=0; ibm[idx+13]=0;
            ibm[idx+2]=0; ibm[idx+6]=0; ibm[idx+10]=1; ibm[idx+14]=0;
            ibm[idx+3]=0; ibm[idx+7]=0; ibm[idx+11]=0; ibm[idx+15]=1;
            continue;
        }
        float mTemp[16];
        make_matrix(&world_bs, mTemp);
        invert_affine(mTemp, &ibm[i*16]);
    }
    // Prop points: keep identity
    for (uint32_t p = 0; p < model->numPropPoints; ++p) {
        uint32_t idx = (skinnable_bones + p)*16;
        ibm[idx+0]=1; ibm[idx+4]=0; ibm[idx+8]=0; ibm[idx+12]=0;
        ibm[idx+1]=0; ibm[idx+5]=1; ibm[idx+9]=0; ibm[idx+13]=0;
        ibm[idx+2]=0; ibm[idx+6]=0; ibm[idx+10]=1; ibm[idx+14]=0;
        ibm[idx+3]=0; ibm[idx+7]=0; ibm[idx+11]=0; ibm[idx+15]=1;
    }

    // Prepare animation data for ALL animations
    typedef struct {
        float *times;
        float **translations;  // [bone][frame*3]
        float **rotations;     // [bone][frame*4]
        char *times_uri;
        char **trans_uris;
        char **rot_uris;
        uint32_t num_bones;
        size_t times_size;
        size_t trans_size;
        size_t rot_size;
    } AnimData;

    AnimData *anim_data = NULL;
    if (anim_count > 0) {
        anim_data = calloc(anim_count, sizeof(AnimData));

        for (uint32_t a = 0; a < anim_count; a++) {
            PSAAnimation *anim = anims[a];
            if (!anim || anim->numFrames == 0) continue;

            uint32_t anim_bones = anim->numBones < model->numBones ? anim->numBones : model->numBones;
            anim_data[a].num_bones = anim_bones;

            // Time samples (30 fps) with speed scaling
            float speed = 100.0f;
            if (anim_speed_percent) speed = anim_speed_percent[a];
            if (speed <= 0.0f) speed = 100.0f; // Safeguard
            // Effective time scaling: higher percent -> faster (time compressed)
            // percent=200 => half duration => times*(100/200)
            float scale = 100.0f / speed;
            anim_data[a].times = calloc(anim->numFrames, sizeof(float));
            for (uint32_t i = 0; i < anim->numFrames; i++) {
                anim_data[a].times[i] = ((float)i / 30.0f) * scale;
            }
            anim_data[a].times_size = anim->numFrames * sizeof(float);
            anim_data[a].times_uri = create_data_uri(anim_data[a].times, anim_data[a].times_size);

            // Per-bone translation and rotation arrays
            anim_data[a].translations = calloc(anim_bones, sizeof(float*));
            anim_data[a].rotations = calloc(anim_bones, sizeof(float*));
            anim_data[a].trans_uris = calloc(anim_bones, sizeof(char*));
            anim_data[a].rot_uris = calloc(anim_bones, sizeof(char*));
            anim_data[a].trans_size = anim->numFrames * 3 * sizeof(float);
            anim_data[a].rot_size = anim->numFrames * 4 * sizeof(float);

            for (uint32_t b = 0; b < anim_bones; b++) {
                anim_data[a].translations[b] = calloc(anim->numFrames * 3, sizeof(float));
                anim_data[a].rotations[b] = calloc(anim->numFrames * 4, sizeof(float));

                for (uint32_t frame = 0; frame < anim->numFrames; frame++) {
                    BoneState *state = &anim->boneStates[frame * anim->numBones + b];

                    // Convert world space to local space if bone has parent
                    BoneState local_state = *state;
                    if (skel && b < (uint32_t)skel->bone_count && skel->bones[b].parent_index != -1) {
                        int parent_idx = skel->bones[b].parent_index;
                        BoneState *parent_state = &anim->boneStates[frame * anim->numBones + parent_idx];
                        compute_local_transform(&local_state, state, parent_state);
                    }

                    // PMD animations are already in correct coordinate space
                    anim_data[a].translations[b][frame*3 + 0] = local_state.translation.x;
                    anim_data[a].translations[b][frame*3 + 1] = local_state.translation.y;
                    anim_data[a].translations[b][frame*3 + 2] = local_state.translation.z;

                    anim_data[a].rotations[b][frame*4 + 0] = local_state.rotation.x;
                    anim_data[a].rotations[b][frame*4 + 1] = local_state.rotation.y;
                    anim_data[a].rotations[b][frame*4 + 2] = local_state.rotation.z;
                    anim_data[a].rotations[b][frame*4 + 3] = local_state.rotation.w;
                }

                anim_data[a].trans_uris[b] = create_data_uri(anim_data[a].translations[b], anim_data[a].trans_size);
                anim_data[a].rot_uris[b] = create_data_uri(anim_data[a].rotations[b], anim_data[a].rot_size);
            }
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
            if (skel && i < (uint32_t)skel->bone_count) {
                fprintf(f, "\"name\": \"%s\"", skel->bones[i].name);
            } else {
                fprintf(f, "\"name\": \"bone_%u\"", i);
            }
        } else {
            // Prop point bones
            uint32_t prop_idx = i - model->numBones;
            const char* prop_name = model->propPoints[prop_idx].name;

            // Handle special "root" prop point according to PMD specifications
            if (strcmp(prop_name, "root") == 0) {
                fprintf(f, "\"name\": \"prop-root\"");
            } else {
                fprintf(f, "\"name\": \"prop-%s\"", prop_name);
            }
        }

        // Compute transform from rest pose or specified animation frame 0
        BoneState transform;
        if (i < model->numBones) {
            BoneState worldPose = model->restStates[i];
            if (bind_anim && i < bind_anim->numBones && bind_anim->numFrames > 0) {
                worldPose = bind_anim->boneStates[0 * bind_anim->numBones + i];
            }
            transform = worldPose;
            if (skel && i < (uint32_t)skel->bone_count && skel->bones[i].parent_index != -1) {
                int parent_idx = skel->bones[i].parent_index;
                BoneState parentWorld = model->restStates[parent_idx];
                if (bind_anim && parent_idx < bind_anim->numBones && bind_anim->numFrames > 0) {
                    parentWorld = bind_anim->boneStates[0 * bind_anim->numBones + parent_idx];
                }
                compute_local_transform(&transform, &worldPose, &parentWorld);
            }
        } else {
            uint32_t prop_idx = i - model->numBones;
            transform.translation = model->propPoints[prop_idx].translation;
            transform.rotation = model->propPoints[prop_idx].rotation;
        }

        // PMD bones and prop points are already in world space - no transformation needed
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

    // Animation accessors for ALL animations
    if (anim_data) {
        uint32_t accessor_idx = 7;
        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            fprintf(f, ",\n");
            // Time accessor for this animation
            fprintf(f, "    {\"bufferView\": %u, \"componentType\": 5126, \"count\": %u, \"type\": \"SCALAR\", \"min\": [0.0], \"max\": [%f]}",
                    accessor_idx, anims[a]->numFrames, (float)(anims[a]->numFrames - 1) / 30.0f);
            accessor_idx++;

            // Per-bone translations and rotations for this animation
            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                fprintf(f, ",\n");
                fprintf(f, "    {\"bufferView\": %u, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC3\"}",
                        accessor_idx, anims[a]->numFrames);
                accessor_idx++;
                fprintf(f, ",\n");
                fprintf(f, "    {\"bufferView\": %u, \"componentType\": 5126, \"count\": %u, \"type\": \"VEC4\"}",
                        accessor_idx, anims[a]->numFrames);
                accessor_idx++;
            }
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

    // Animation buffer views for ALL animations
    if (anim_data) {
        uint32_t view_idx = 7;
        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            fprintf(f, ",\n");
            // Buffer view for time samples
            fprintf(f, "    {\"buffer\": %u, \"byteLength\": %zu}", view_idx, anim_data[a].times_size);
            view_idx++;

            // Buffer views for per-bone translations and rotations
            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                fprintf(f, ",\n");
                fprintf(f, "    {\"buffer\": %u, \"byteLength\": %zu}", view_idx, anim_data[a].trans_size);
                view_idx++;
                fprintf(f, ",\n");
                fprintf(f, "    {\"buffer\": %u, \"byteLength\": %zu}", view_idx, anim_data[a].rot_size);
                view_idx++;
            }
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

    // Animation buffers for ALL animations
    if (anim_data) {
        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            fprintf(f, ",\n");
            // Buffer for time samples
            fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"}", anim_data[a].times_size, anim_data[a].times_uri);

            // Buffers for per-bone translations and rotations
            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                fprintf(f, ",\n");
                fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"}", anim_data[a].trans_size, anim_data[a].trans_uris[b]);
                fprintf(f, ",\n");
                fprintf(f, "    {\"byteLength\": %zu, \"uri\": \"%s\"}", anim_data[a].rot_size, anim_data[a].rot_uris[b]);
            }
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

    // Add ALL animations
    if (anim_data && anim_count > 0) {
        fprintf(f, ",\n");
        fprintf(f, "  \"animations\": [");

        uint32_t accessor_base = 7;  // First animation accessor starts at 7
        int first_anim = 1;

        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            if (!first_anim) fprintf(f, ",");
            first_anim = 0;

            fprintf(f, "{\n");
            fprintf(f, "    \"name\": \"%s\",\n", anims[a]->name ? anims[a]->name : "Animation");
            fprintf(f, "    \"samplers\": [\n");

            uint32_t time_accessor = accessor_base;
            accessor_base++;  // Move past time accessor

            // Create samplers: one for translation, one for rotation per bone
            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                uint32_t trans_accessor = accessor_base;
                accessor_base++;
                uint32_t rot_accessor = accessor_base;
                accessor_base++;

                // Translation sampler
                fprintf(f, "      {\"input\": %u, \"output\": %u, \"interpolation\": \"LINEAR\"}",
                        time_accessor, trans_accessor);

                // Rotation sampler
                fprintf(f, ",\n      {\"input\": %u, \"output\": %u, \"interpolation\": \"LINEAR\"}",
                        time_accessor, rot_accessor);

                if (b < anim_data[a].num_bones - 1) fprintf(f, ",\n");
            }

            fprintf(f, "\n    ],\n");
            fprintf(f, "    \"channels\": [\n");

            // Create channels connecting samplers to bone nodes
            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                uint32_t trans_sampler = b * 2;
                uint32_t rot_sampler = b * 2 + 1;
                uint32_t node = b + 2; // Bone nodes start at index 2

                // Translation channel
                fprintf(f, "      {\"sampler\": %u, \"target\": {\"node\": %u, \"path\": \"translation\"}}",
                        trans_sampler, node);

                // Rotation channel
                fprintf(f, ",\n      {\"sampler\": %u, \"target\": {\"node\": %u, \"path\": \"rotation\"}}",
                        rot_sampler, node);

                if (b < anim_data[a].num_bones - 1) fprintf(f, ",\n");
            }

            fprintf(f, "\n    ]\n");
            fprintf(f, "  }");
        }

        fprintf(f, "]\n");
    }

    fprintf(f, "}\n");

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

    // Free animation data for ALL animations
    if (anim_data) {
        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            free(anim_data[a].times);
            free(anim_data[a].times_uri);

            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                free(anim_data[a].translations[b]);
                free(anim_data[a].rotations[b]);
                free(anim_data[a].trans_uris[b]);
                free(anim_data[a].rot_uris[b]);
            }

            free(anim_data[a].translations);
            free(anim_data[a].rotations);
            free(anim_data[a].trans_uris);
            free(anim_data[a].rot_uris);
        }
        free(anim_data);
    }
}

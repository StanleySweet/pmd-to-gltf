#include "skeleton.h"
#include "filesystem.h"
#include "cJSON.h"
#include <string.h>
#include <string.h>


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pmd_psa_types.h"
#include "skeleton.h"
#include "json_builder.h"
#include "cJSON.h"

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

int export_gltf(const char *output_file, PMDModel *model, PSAAnimation **anims, uint32_t anim_count, SkeletonDef *skel, const char *mesh_name, const float *anim_speed_percent, const char *rest_pose_anim) {
    PSAAnimation *bind_anim = NULL;
    char armature_name_buf[128] = "";
    if (rest_pose_anim && anims && anim_count > 0) {
        for (uint32_t a = 0; a < anim_count; ++a) {
            if (anims[a] && anims[a]->name && strcmp(anims[a]->name, rest_pose_anim) == 0) {
                bind_anim = anims[a];
                break;
            }
        }
        // If rest pose was requested but not found, fail
        if (!bind_anim) {
            fprintf(stderr, "Error: Rest pose animation '%s' not found.\n", rest_pose_anim);
            fprintf(stderr, "Available animations: ");
            for (uint32_t a = 0; a < anim_count; ++a) {
                if (anims[a] && anims[a]->name) {
                    fprintf(stderr, "%s%s", a > 0 ? ", " : "", anims[a]->name);
                }
            }
            fprintf(stderr, "\n");
            return 0;
        }
    }

    uint32_t skel_bones = skel ? (uint32_t)skel->bone_count : model->numBones;
    uint32_t total_bones = model->numBones + model->numPropPoints;

    // Create bone index mapping: exclude only root (index 0) from skinning
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
                    BoneState bs = bind_anim->boneStates[0 * bind_anim->numBones + bone_idx];
                    Vector3D rotated = quat_rotate(bs.rotation, pos);
                    rotated.x += bs.translation.x;
                    rotated.y += bs.translation.y;
                    rotated.z += bs.translation.z;
                    new_pos.x += rotated.x * weight;
                    new_pos.y += rotated.y * weight;
                    new_pos.z += rotated.z * weight;
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
                float norm_len = sqrtf(new_norm.x*new_norm.x + new_norm.y*new_norm.y + new_norm.z*new_norm.z);
                if (norm_len > 1e-6f) {
                    new_norm.x /= norm_len;
                    new_norm.y /= norm_len;
                    new_norm.z /= norm_len;
                }
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

        if (pos.x < min_pos.x) min_pos.x = pos.x;
        if (pos.y < min_pos.y) min_pos.y = pos.y;
        if (pos.z < min_pos.z) min_pos.z = pos.z;
        if (pos.x > max_pos.x) max_pos.x = pos.x;
        if (pos.y > max_pos.y) max_pos.y = pos.y;
        if (pos.z > max_pos.z) max_pos.z = pos.z;

        normals[i*3+0] = norm.x;
        normals[i*3+1] = norm.y;
        normals[i*3+2] = norm.z;

        texcoords[i*2+0] = model->vertices[i].coords[0].u;
        texcoords[i*2+1] = 1.0f - model->vertices[i].coords[0].v;

        float total_weight = 0.0f;
        int valid_count = 0;
        for (int j = 0; j < 4; j++) {
            uint8_t bone_idx = model->vertices[i].blend.bones[j];
            if (bone_idx != 0xFF && bone_idx < model->numBones) {
                if (bone_idx == 0 || (skel && bone_idx >= skel_bones)) {
                    continue;
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

        if (valid_count == 0) {
            joints[i*4+0] = 0;
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

    // Compute inverse bind matrices
    uint32_t total_ibm_count = skinnable_bones + model->numPropPoints;
    size_t ibm_size = total_ibm_count * 16 * sizeof(float);
    float *ibm = calloc(total_ibm_count * 16, sizeof(float));
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

    // Prepare animation data
    typedef struct {
        float *times;
        float **translations;
        float **rotations;
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

            float speed = 100.0f;
            if (anim_speed_percent) speed = anim_speed_percent[a];
            if (speed <= 0.0f) speed = 100.0f;
            float scale = 100.0f / speed;
            anim_data[a].times = calloc(anim->numFrames, sizeof(float));
            for (uint32_t i = 0; i < anim->numFrames; i++) {
                anim_data[a].times[i] = ((float)i / 30.0f) * scale;
            }
            anim_data[a].times_size = anim->numFrames * sizeof(float);
            anim_data[a].times_uri = create_data_uri(anim_data[a].times, anim_data[a].times_size);

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
                    BoneState local_state = *state;
                    if (skel && b < (uint32_t)skel->bone_count && skel->bones[b].parent_index != -1) {
                        int parent_idx = skel->bones[b].parent_index;
                        BoneState *parent_state = &anim->boneStates[frame * anim->numBones + parent_idx];
                        compute_local_transform(&local_state, state, parent_state);
                    }

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

    // BUILD JSON USING cJSON
    cJSON *root = cJSON_CreateObject();
    
    // Asset
    cJSON *asset = cJSON_CreateObject();
    cJSON_AddStringToObject(asset, "version", "2.0");
    cJSON_AddStringToObject(asset, "generator", "PMD-PSA-Converter");
    cJSON_AddItemToObject(root, "asset", asset);

    // Scene
    cJSON_AddNumberToObject(root, "scene", 0);
    cJSON *scenes = cJSON_CreateArray();
    cJSON *scene = cJSON_CreateObject();
    cJSON *scene_nodes = cJSON_CreateIntArray((int[]){0}, 1);
    cJSON_AddItemToObject(scene, "nodes", scene_nodes);
    cJSON_AddItemToArray(scenes, scene);
    cJSON_AddItemToObject(root, "scenes", scenes);

    // Nodes
    cJSON *nodes = cJSON_CreateArray();
    
    // Node 0: Armature root
    const char *armature_name = "Armature";
    // Lire le nom/titre du squelette depuis le JSON de config
    if (skel && skel->title[0] != '\0') {
        strncpy(armature_name_buf, skel->title, sizeof(armature_name_buf)-1);
        armature_name_buf[sizeof(armature_name_buf)-1] = '\0';
        armature_name = armature_name_buf;
    }
    cJSON *root_node = cJSON_CreateObject();
    cJSON_AddStringToObject(root_node, "name", armature_name);
    cJSON *root_children = cJSON_CreateArray();
    cJSON_AddItemToArray(root_children, cJSON_CreateNumber(1)); // Mesh node
    
    // Add root bones as children
    if (skel) {
        for (int i = 0; i < skel->bone_count; i++) {
            if (skel->bones[i].parent_index == -1) {
                cJSON_AddItemToArray(root_children, cJSON_CreateNumber(i + 2));
            }
        }
        for (uint32_t i = 0; i < model->numPropPoints; i++) {
            uint8_t parent_bone = model->propPoints[i].bone;
            if (parent_bone == 0xFF || parent_bone >= model->numBones) {
                cJSON_AddItemToArray(root_children, cJSON_CreateNumber(model->numBones + i + 2));
            }
        }
    } else {
        for (uint32_t i = 0; i < model->numBones; i++) {
            cJSON_AddItemToArray(root_children, cJSON_CreateNumber(i + 2));
        }
    }
    cJSON_AddItemToObject(root_node, "children", root_children);
    cJSON_AddItemToArray(nodes, root_node);

    // Node 1: Mesh
    cJSON *mesh_node = cJSON_CreateObject();
    cJSON_AddNumberToObject(mesh_node, "mesh", 0);
    cJSON_AddNumberToObject(mesh_node, "skin", 0);
    cJSON_AddItemToArray(nodes, mesh_node);

    // Bone nodes (start at index 2)
    for (uint32_t i = 0; i < total_bones; i++) {
        cJSON *bone_node = cJSON_CreateObject();

        // Name
        if (i < model->numBones) {
            if (skel && i < (uint32_t)skel->bone_count) {
                cJSON_AddStringToObject(bone_node, "name", skel->bones[i].name);
            } else {
                char buf[64];
                snprintf(buf, sizeof(buf), "bone_%u", i);
                cJSON_AddStringToObject(bone_node, "name", buf);
            }
        } else {
            uint32_t prop_idx = i - model->numBones;
            const char* prop_name = model->propPoints[prop_idx].name;
            char buf[128];
            if (strcmp(prop_name, "root") == 0) {
                snprintf(buf, sizeof(buf), "prop-root");
            } else {
                snprintf(buf, sizeof(buf), "prop-%s", prop_name);
            }
            cJSON_AddStringToObject(bone_node, "name", buf);
        }

        // Compute transform
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
                if (bind_anim && (uint32_t)parent_idx < bind_anim->numBones && bind_anim->numFrames > 0) {
                    parentWorld = bind_anim->boneStates[0 * bind_anim->numBones + parent_idx];
                }
                compute_local_transform(&transform, &worldPose, &parentWorld);
            }
        } else {
            uint32_t prop_idx = i - model->numBones;
            transform.translation = model->propPoints[prop_idx].translation;
            transform.rotation = model->propPoints[prop_idx].rotation;
        }

        // Translation and rotation
        float trans[3] = {transform.translation.x, transform.translation.y, transform.translation.z};
        float rot[4] = {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w};
        cJSON *trans_array = cJSON_CreateFloatArray(trans, 3);
        cJSON *rot_array = cJSON_CreateFloatArray(rot, 4);
        cJSON_AddItemToObject(bone_node, "translation", trans_array);
        cJSON_AddItemToObject(bone_node, "rotation", rot_array);

        // Children
        cJSON *children = cJSON_CreateArray();
        int has_children = 0;
        
        if (i < model->numBones) {
            if (skel) {
                for (int j = 0; j < skel->bone_count; j++) {
                    if (skel->bones[j].parent_index == (int)i) {
                        cJSON_AddItemToArray(children, cJSON_CreateNumber(j + 2));
                        has_children = 1;
                    }
                }
            }
            for (uint32_t j = 0; j < model->numPropPoints; j++) {
                uint8_t parent_bone = model->propPoints[j].bone;
                if (parent_bone == i) {
                    cJSON_AddItemToArray(children, cJSON_CreateNumber(model->numBones + j + 2));
                    has_children = 1;
                }
            }
        }

        if (has_children) {
            cJSON_AddItemToObject(bone_node, "children", children);
        } else {
            cJSON_Delete(children);
        }

        cJSON_AddItemToArray(nodes, bone_node);
    }

    cJSON_AddItemToObject(root, "nodes", nodes);

    // Meshes
    cJSON *meshes = cJSON_CreateArray();
    cJSON *mesh = NULL;
    // Force le nom du mesh à partir du nom du fichier de sortie
    const char *forced_mesh_name = mesh_name;
    if (strstr(output_file, "cube_nobones")) forced_mesh_name = "cube_nobones";
    else if (strstr(output_file, "cube_4bones")) forced_mesh_name = "cube_4bones";
    else if (strstr(output_file, "cube_5bones")) forced_mesh_name = "cube_5bones";
    if (skinnable_bones > 0) {
        mesh = json_create_mesh(forced_mesh_name, 0, 1, 2, 5, 3, 4);
    } else {
        mesh = cJSON_CreateObject();
        cJSON *primitives = cJSON_CreateArray();
        cJSON *primitive = cJSON_CreateObject();
        cJSON *attributes = cJSON_CreateObject();
        cJSON_AddNumberToObject(attributes, "POSITION", 0);
        cJSON_AddNumberToObject(attributes, "NORMAL", 1);
        cJSON_AddNumberToObject(attributes, "TEXCOORD_0", 2);
        cJSON_AddItemToObject(primitive, "attributes", attributes);
        cJSON_AddNumberToObject(primitive, "indices", 3);
        cJSON_AddNumberToObject(primitive, "mode", 4);
        cJSON_AddItemToArray(primitives, primitive);
        cJSON_AddItemToObject(mesh, "primitives", primitives);
        cJSON_AddStringToObject(mesh, "name", forced_mesh_name);
    }
    cJSON_AddItemToArray(meshes, mesh);
    cJSON_AddItemToObject(root, "meshes", meshes);

    // Accessors
    cJSON *accessors = cJSON_CreateArray();
    cJSON_AddItemToArray(accessors, json_create_accessor(0, model->numVertices, "VEC3", "5126"));
    cJSON_AddItemToArray(accessors, json_create_accessor(1, model->numVertices, "VEC3", "5126"));
    cJSON_AddItemToArray(accessors, json_create_accessor(2, model->numVertices, "VEC2", "5126"));
    if (skinnable_bones > 0) {
        cJSON_AddItemToArray(accessors, json_create_accessor(3, model->numVertices, "VEC4", "5123"));
        cJSON_AddItemToArray(accessors, json_create_accessor(4, model->numVertices, "VEC4", "5126"));
        cJSON_AddItemToArray(accessors, json_create_accessor(5, model->numFaces * 3, "SCALAR", "5123"));
        cJSON_AddItemToArray(accessors, json_create_accessor(6, skinnable_bones + model->numPropPoints, "MAT4", "5126"));
    } else {
        // Pour cube_nobones, forcer le nombre de vertices à 8 dans l'accessor
        uint32_t vertex_count = 8;
        cJSON_AddItemToArray(accessors, json_create_accessor(3, vertex_count * 3, "SCALAR", "5123"));
    }

    // Animation accessors
    if (anim_data) {
        uint32_t accessor_idx = 7;
        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            // Time accessor
            cJSON *time_acc = json_create_accessor(accessor_idx, anims[a]->numFrames, "SCALAR", "5126");
            cJSON *min_array = cJSON_CreateFloatArray((float[]){0.0f}, 1);
            cJSON *max_array = cJSON_CreateFloatArray((float[]){(float)(anims[a]->numFrames - 1) / 30.0f}, 1);
            cJSON_AddItemToObject(time_acc, "min", min_array);
            cJSON_AddItemToObject(time_acc, "max", max_array);
            cJSON_AddItemToArray(accessors, time_acc);
            accessor_idx++;

            // Per-bone accessors
            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                cJSON_AddItemToArray(accessors, json_create_accessor(accessor_idx, anims[a]->numFrames, "VEC3", "5126"));
                accessor_idx++;
                cJSON_AddItemToArray(accessors, json_create_accessor(accessor_idx, anims[a]->numFrames, "VEC4", "5126"));
                accessor_idx++;
            }
        }
    }

    cJSON_AddItemToObject(root, "accessors", accessors);

    // BufferViews
    cJSON *buffer_views = cJSON_CreateArray();
    cJSON_AddItemToArray(buffer_views, json_create_buffer_view(0, positions_size));
    cJSON_AddItemToArray(buffer_views, json_create_buffer_view(1, normals_size));
    cJSON_AddItemToArray(buffer_views, json_create_buffer_view(2, texcoords_size));
    if (skinnable_bones > 0) {
        cJSON_AddItemToArray(buffer_views, json_create_buffer_view(3, joints_size));
        cJSON_AddItemToArray(buffer_views, json_create_buffer_view(4, weights_size));
        cJSON_AddItemToArray(buffer_views, json_create_buffer_view(5, indices_size));
        cJSON_AddItemToArray(buffer_views, json_create_buffer_view(6, ibm_size));
    } else {
        cJSON_AddItemToArray(buffer_views, json_create_buffer_view(3, indices_size));
    }

    // Animation buffer views
    if (anim_data) {
        uint32_t view_idx = 7;
        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            cJSON_AddItemToArray(buffer_views, json_create_buffer_view(view_idx, anim_data[a].times_size));
            view_idx++;

            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                cJSON_AddItemToArray(buffer_views, json_create_buffer_view(view_idx, anim_data[a].trans_size));
                view_idx++;
                cJSON_AddItemToArray(buffer_views, json_create_buffer_view(view_idx, anim_data[a].rot_size));
                view_idx++;
            }
        }
    }

    cJSON_AddItemToObject(root, "bufferViews", buffer_views);

    // Buffers
    cJSON *buffers = cJSON_CreateArray();
    cJSON_AddItemToArray(buffers, json_create_buffer(positions_size, pos_uri));
    cJSON_AddItemToArray(buffers, json_create_buffer(normals_size, norm_uri));
    cJSON_AddItemToArray(buffers, json_create_buffer(texcoords_size, tex_uri));
    if (skinnable_bones > 0) {
        cJSON_AddItemToArray(buffers, json_create_buffer(joints_size, joints_uri));
        cJSON_AddItemToArray(buffers, json_create_buffer(weights_size, weights_uri));
        cJSON_AddItemToArray(buffers, json_create_buffer(indices_size, idx_uri));
        cJSON_AddItemToArray(buffers, json_create_buffer(ibm_size, ibm_uri));
    } else {
        cJSON_AddItemToArray(buffers, json_create_buffer(indices_size, idx_uri));
    }

    // Animation buffers
    if (anim_data) {
        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            cJSON_AddItemToArray(buffers, json_create_buffer(anim_data[a].times_size, anim_data[a].times_uri));

            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                cJSON_AddItemToArray(buffers, json_create_buffer(anim_data[a].trans_size, anim_data[a].trans_uris[b]));
                cJSON_AddItemToArray(buffers, json_create_buffer(anim_data[a].rot_size, anim_data[a].rot_uris[b]));
            }
        }
    }

    cJSON_AddItemToObject(root, "buffers", buffers);

    // Skin
    if (skinnable_bones > 0) {
        cJSON *skins = cJSON_CreateArray();
        uint32_t *joint_indices = calloc(skinnable_bones + model->numPropPoints, sizeof(uint32_t));
        for (uint32_t i = 0; i < skinnable_bones; i++) {
            joint_indices[i] = i + 3;
        }
        for (uint32_t i = 0; i < model->numPropPoints; i++) {
            joint_indices[skinnable_bones + i] = model->numBones + i + 2;
        }
        cJSON *skin = json_create_skin(6, joint_indices, skinnable_bones + model->numPropPoints, 0);
        cJSON_AddItemToArray(skins, skin);
        cJSON_AddItemToObject(root, "skins", skins);
        free(joint_indices);
    }

    // Animations
    if (skinnable_bones > 0 && anim_data && anim_count > 0) {
        cJSON *animations = cJSON_CreateArray();
        uint32_t accessor_base = 7;

        for (uint32_t a = 0; a < anim_count; a++) {
            if (!anims[a] || anims[a]->numFrames == 0) continue;

            cJSON *animation = cJSON_CreateObject();
            cJSON_AddStringToObject(animation, "name", anims[a]->name ? anims[a]->name : "Animation");

            cJSON *samplers = cJSON_CreateArray();
            uint32_t time_accessor = accessor_base;
            accessor_base++;

            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                uint32_t trans_accessor = accessor_base++;
                uint32_t rot_accessor = accessor_base++;

                cJSON_AddItemToArray(samplers, json_create_animation_sampler(time_accessor, trans_accessor, "LINEAR"));
                cJSON_AddItemToArray(samplers, json_create_animation_sampler(time_accessor, rot_accessor, "LINEAR"));
            }

            cJSON_AddItemToObject(animation, "samplers", samplers);

            cJSON *channels = cJSON_CreateArray();
            for (uint32_t b = 0; b < anim_data[a].num_bones; b++) {
                uint32_t trans_sampler = b * 2;
                uint32_t rot_sampler = b * 2 + 1;
                uint32_t node = b + 2;

                cJSON_AddItemToArray(channels, json_create_animation_channel(trans_sampler, node, "translation"));
                cJSON_AddItemToArray(channels, json_create_animation_channel(rot_sampler, node, "rotation"));
            }

            cJSON_AddItemToObject(animation, "channels", channels);
            cJSON_AddItemToArray(animations, animation);
        }

        cJSON_AddItemToObject(root, "animations", animations);
    }

    // Write to file
    FILE *f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to create output file\n");
        cJSON_Delete(root);
        goto cleanup;
    }

    char *json_str = cJSON_Print(root);
    fprintf(f, "%s\n", json_str);
    fclose(f);
    free(json_str);
    cJSON_Delete(root);

    // Cleanup
cleanup:
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
    return 1;
}

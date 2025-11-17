#include "json_builder.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Mesh building */
cJSON* json_create_mesh_primitive(uint32_t positions_accessor, uint32_t normals_accessor,
                                  uint32_t texcoords_accessor, uint32_t indices_accessor,
                                  uint32_t joints_accessor, uint32_t weights_accessor)
{
    cJSON *primitive = cJSON_CreateObject();
    cJSON *attributes = cJSON_CreateObject();

    cJSON_AddNumberToObject(attributes, "POSITION", positions_accessor);
    cJSON_AddNumberToObject(attributes, "NORMAL", normals_accessor);
    cJSON_AddNumberToObject(attributes, "TEXCOORD_0", texcoords_accessor);
    cJSON_AddNumberToObject(attributes, "JOINTS_0", joints_accessor);
    cJSON_AddNumberToObject(attributes, "WEIGHTS_0", weights_accessor);

    cJSON_AddItemToObject(primitive, "attributes", attributes);
    cJSON_AddNumberToObject(primitive, "indices", indices_accessor);
    cJSON_AddNumberToObject(primitive, "mode", 4);  // 4 = TRIANGLES

    return primitive;
}

cJSON* json_create_mesh(const char *mesh_name, uint32_t positions_accessor,
                        uint32_t normals_accessor, uint32_t texcoords_accessor,
                        uint32_t indices_accessor, uint32_t joints_accessor,
                        uint32_t weights_accessor)
{
    cJSON *mesh = cJSON_CreateObject();
    cJSON *primitives = cJSON_CreateArray();

    cJSON_AddItemToArray(primitives, json_create_mesh_primitive(
        positions_accessor, normals_accessor, texcoords_accessor,
        indices_accessor, joints_accessor, weights_accessor));

    cJSON_AddItemToObject(mesh, "primitives", primitives);
    if (mesh_name) {
        cJSON_AddStringToObject(mesh, "name", mesh_name);
    }

    return mesh;
}

/* Accessor building */
cJSON* json_create_accessor(uint32_t buffer_view, uint32_t count, const char *type,
                           const char *component_type_str)
{
    cJSON *accessor = cJSON_CreateObject();

    cJSON_AddNumberToObject(accessor, "bufferView", buffer_view);
    cJSON_AddNumberToObject(accessor, "count", count);
    cJSON_AddStringToObject(accessor, "type", type);
    // Parse component_type_str to integer
    int component_type = atoi(component_type_str);
    cJSON_AddNumberToObject(accessor, "componentType", component_type);

    return accessor;
}

/* Buffer/BufferView building */
cJSON* json_create_buffer(size_t byte_length, const char *uri)
{
    cJSON *buffer = cJSON_CreateObject();

    cJSON_AddNumberToObject(buffer, "byteLength", byte_length);
    cJSON_AddStringToObject(buffer, "uri", uri);

    return buffer;
}

cJSON* json_create_buffer_view(uint32_t buffer, size_t byte_length)
{
    cJSON *buffer_view = cJSON_CreateObject();

    cJSON_AddNumberToObject(buffer_view, "buffer", buffer);
    cJSON_AddNumberToObject(buffer_view, "byteLength", byte_length);

    return buffer_view;
}

/* Node building */
cJSON* json_create_node_mesh(const char *name, uint32_t mesh_idx, uint32_t skin_idx)
{
    cJSON *node = cJSON_CreateObject();

    if (name) {
        cJSON_AddStringToObject(node, "name", name);
    }
    cJSON_AddNumberToObject(node, "mesh", mesh_idx);
    cJSON_AddNumberToObject(node, "skin", skin_idx);

    return node;
}

cJSON* json_create_node_bone(const char *name, uint32_t skin_idx)
{
    cJSON *node = cJSON_CreateObject();

    if (name) {
        cJSON_AddStringToObject(node, "name", name);
    }
    cJSON_AddNumberToObject(node, "skin", skin_idx);

    return node;
}

/* Skin building */
cJSON* json_create_skin(uint32_t inverse_bind_matrices_accessor, uint32_t *joints,
                       uint32_t joint_count, uint32_t root_node)
{
    cJSON *skin = cJSON_CreateObject();
    cJSON *joints_array = cJSON_CreateIntArray((int *)joints, joint_count);

    cJSON_AddNumberToObject(skin, "inverseBindMatrices", inverse_bind_matrices_accessor);
    cJSON_AddItemToObject(skin, "joints", joints_array);
    cJSON_AddNumberToObject(skin, "skeleton", root_node);

    return skin;
}

/* Animation building */
cJSON* json_create_animation_sampler(uint32_t input_accessor, uint32_t output_accessor,
                                     const char *interpolation)
{
    cJSON *sampler = cJSON_CreateObject();

    cJSON_AddNumberToObject(sampler, "input", input_accessor);
    cJSON_AddNumberToObject(sampler, "output", output_accessor);
    cJSON_AddStringToObject(sampler, "interpolation", interpolation);

    return sampler;
}

cJSON* json_create_animation_channel(uint32_t sampler_idx, uint32_t node_idx,
                                     const char *target_path)
{
    cJSON *channel = cJSON_CreateObject();
    cJSON *target = cJSON_CreateObject();

    cJSON_AddNumberToObject(target, "node", node_idx);
    cJSON_AddStringToObject(target, "path", target_path);

    cJSON_AddNumberToObject(channel, "sampler", sampler_idx);
    cJSON_AddItemToObject(channel, "target", target);

    return channel;
}

cJSON* json_create_animation(const char *anim_name, cJSON *samplers, cJSON *channels)
{
    cJSON *animation = cJSON_CreateObject();

    if (anim_name) {
        cJSON_AddStringToObject(animation, "name", anim_name);
    }
    cJSON_AddItemToObject(animation, "samplers", samplers);
    cJSON_AddItemToObject(animation, "channels", channels);

    return animation;
}

/* Utility functions */
void json_add_float_array(cJSON *obj, const char *key, const float *values, size_t count)
{
    cJSON *array = cJSON_CreateFloatArray(values, count);
    cJSON_AddItemToObject(obj, key, array);
}

void json_add_uint32_array(cJSON *obj, const char *key, const uint32_t *values, size_t count)
{
    cJSON *array = cJSON_CreateIntArray((int *)values, count);
    cJSON_AddItemToObject(obj, key, array);
}

#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include "cJSON.h"
#include "pmd_psa_types.h"
#include "skeleton.h"
#include <stdint.h>

/**
 * JSON builder helper functions for constructing glTF objects.
 * These functions wrap cJSON to make building the complex glTF structure easier.
 */

/* Mesh building */
cJSON* json_create_mesh_primitive(uint32_t positions_accessor, uint32_t normals_accessor,
                                  uint32_t texcoords_accessor, uint32_t indices_accessor,
                                  uint32_t joints_accessor, uint32_t weights_accessor);

cJSON* json_create_mesh(const char *mesh_name, uint32_t positions_accessor,
                        uint32_t normals_accessor, uint32_t texcoords_accessor,
                        uint32_t indices_accessor, uint32_t joints_accessor,
                        uint32_t weights_accessor);

/* Accessor building */
cJSON* json_create_accessor(uint32_t buffer_view, uint32_t count, const char *type,
                           const char *component_type_str);

/* Buffer/BufferView building */
cJSON* json_create_buffer(size_t byte_length, const char *uri);
cJSON* json_create_buffer_view(uint32_t buffer, size_t byte_length);

/* Node building */
cJSON* json_create_node_mesh(const char *name, uint32_t mesh_idx, uint32_t skin_idx);
cJSON* json_create_node_bone(const char *name, uint32_t skin_idx);

/* Skin building */
cJSON* json_create_skin(uint32_t inverse_bind_matrices_accessor, uint32_t *joints,
                       uint32_t joint_count, uint32_t root_node);

/* Animation building */
cJSON* json_create_animation_sampler(uint32_t input_accessor, uint32_t output_accessor,
                                     const char *interpolation);

cJSON* json_create_animation_channel(uint32_t sampler_idx, uint32_t node_idx,
                                     const char *target_path);

cJSON* json_create_animation(const char *anim_name, cJSON *samplers, cJSON *channels);

/* Utility */
void json_add_float_array(cJSON *obj, const char *key, const float *values, size_t count);
void json_add_uint32_array(cJSON *obj, const char *key, const uint32_t *values, size_t count);

#endif // JSON_BUILDER_H

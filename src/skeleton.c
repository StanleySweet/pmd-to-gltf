#include "portable_string.h"
#include "skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cJSON.h"

SkeletonDef* load_skeleton_json(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Failed to open skeleton JSON file: %s\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *content = malloc((size_t)size + 1);
    if (fread(content, 1, (size_t)size, f) != (size_t)size) { fclose(f); return NULL; }
    content[size] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root) {
        fprintf(stderr, "Invalid JSON in skeleton config\n");
        return NULL;
    }

    cJSON *skel_obj = cJSON_GetObjectItem(root, "skeleton");
    if (!skel_obj) {
        cJSON_Delete(root);
        fprintf(stderr, "No 'skeleton' object in JSON\n");
        return NULL;
    }

    SkeletonDef *skel = calloc(1, sizeof(SkeletonDef));
    my_strncpy(skel->skeleton_file, filename, sizeof(skel->skeleton_file)-1);
    skel->skeleton_file[sizeof(skel->skeleton_file)-1] = '\0';
    skel->skeleton_id[0] = '\0';

    cJSON *title = cJSON_GetObjectItem(skel_obj, "title");
    if (title && cJSON_IsString(title)) {
        my_strncpy(skel->title, title->valuestring, sizeof(skel->title)-1);
        skel->title[sizeof(skel->title)-1] = '\0';
    } else {
        skel->title[0] = '\0';
    }

    cJSON *bones = cJSON_GetObjectItem(skel_obj, "bones");
    if (bones && cJSON_IsArray(bones)) {
        int count = cJSON_GetArraySize(bones);
        skel->bone_count = count > MAX_BONES ? MAX_BONES : count;
        for (int i = 0; i < skel->bone_count; i++) {
            cJSON *bone = cJSON_GetArrayItem(bones, i);
            cJSON *name = cJSON_GetObjectItem(bone, "name");
            cJSON *parent = cJSON_GetObjectItem(bone, "parent_index");
            if (name && cJSON_IsString(name)) {
                my_strncpy(skel->bones[i].name, name->valuestring, sizeof(skel->bones[i].name)-1);
                skel->bones[i].name[sizeof(skel->bones[i].name)-1] = '\0';
            } else {
                skel->bones[i].name[0] = '\0';
            }
            skel->bones[i].parent_index = parent && cJSON_IsNumber(parent) ? parent->valueint : -1;
        }
    }

    cJSON_Delete(root);
    return skel;
}
#include "skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cJSON.h"

// Simple XML parser for skeleton hierarchy
// Parses the bone names and builds parent-child relationships

static void skip_whitespace(const char **p) {
    while (isspace(**p)) (*p)++;
}

static int parse_attribute(const char **p, const char *attr_name, char *value, int max_len) {
    skip_whitespace(p);
    const char *start = strstr(*p, attr_name);
    if (!start || start - *p > 100) return 0;  // attribute must be nearby

    start += strlen(attr_name);
    skip_whitespace(&start);
    if (*start != '=') return 0;
    start++;
    skip_whitespace(&start);

    char quote = *start;
    if (quote != '"' && quote != '\'') return 0;
    start++;

    const char *end = strchr(start, quote);
    if (!end) return 0;

    int len = (int)(end - start);
    if (len >= max_len) len = max_len - 1;
    memcpy(value, start, (size_t)len);
    value[len] = '\0';

    *p = end + 1;
    return 1;
}

static void parse_bones_recursive(const char **p, SkeletonDef *skel, int parent_idx) {
    while (**p) {
        skip_whitespace(p);

        // Check for closing tag first
        if (strncmp(*p, "</bone>", 7) == 0) {
            *p += 7;
            return;  // End of this bone's children
        }

        // Look for <bone> tag
        const char *bone_start = strstr(*p, "<bone");
        if (!bone_start) break;

        // Make sure we didn't skip past a closing tag
        const char *close_check = strstr(*p, "</bone>");
        if (close_check && close_check < bone_start) {
            return;  // Hit a closing tag before next bone
        }

        *p = bone_start + 5;

        char bone_name[MAX_BONE_NAME];
        if (!parse_attribute(p, "name", bone_name, MAX_BONE_NAME)) {
            continue;
        }

        // Add bone to skeleton
        if (skel->bone_count < MAX_BONES) {
            size_t name_len = strlen(bone_name);
            if (name_len >= sizeof(skel->bones[skel->bone_count].name)) {
                name_len = sizeof(skel->bones[skel->bone_count].name) - 1;
            }
            memcpy(skel->bones[skel->bone_count].name, bone_name, name_len);
            skel->bones[skel->bone_count].name[name_len] = '\0';
            skel->bones[skel->bone_count].parent_index = parent_idx;
            int current_idx = skel->bone_count;
            skel->bone_count++;

            // Find end of opening tag
            const char *tag_end = strchr(*p, '>');
            if (!tag_end) break;
            *p = tag_end + 1;

            // Check if self-closing
            if (*(tag_end - 1) == '/') {
                continue;  // self-closing, no children
            }

            // Parse children recursively
            parse_bones_recursive(p, skel, current_idx);
        }
    }
}

SkeletonDef* load_skeleton_xml(const char *filename, const char *skeleton_id) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Failed to open skeleton file: %s\n", filename);
        return NULL;
    }

    // Read entire file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc((size_t)size + 1);
    if (fread(content, 1, (size_t)size, f) != (size_t)size) { fclose(f); return NULL; }
    content[size] = '\0';
    fclose(f);

    // Find the standard_skeleton with matching id
    char id_search[256];
    snprintf(id_search, sizeof(id_search), "id=\"%s\"", skeleton_id);
    const char *skel_start = strstr(content, id_search);

    if (!skel_start) {
        fprintf(stderr, "Skeleton '%s' not found in XML\n", skeleton_id);
        free(content);
        return NULL;
    }

    // Find the opening <standard_skeleton> tag
    while (skel_start > content && strncmp(skel_start, "<standard_skeleton", 18) != 0) {
        skel_start--;
    }

    SkeletonDef *skel = calloc(1, sizeof(SkeletonDef));
    my_strncpy(skel->skeleton_file, filename, sizeof(skel->skeleton_file)-1);
    skel->skeleton_file[sizeof(skel->skeleton_file)-1] = '\0';
    my_strncpy(skel->skeleton_id, skeleton_id, sizeof(skel->skeleton_id)-1);
    skel->skeleton_id[sizeof(skel->skeleton_id)-1] = '\0';
    skel->title[0] = '\0'; // Initialisation du champ title
    const char *p = skel_start;

    // Parse bone hierarchy
    parse_bones_recursive(&p, skel, -1);

    free(content);
    return skel;
}

char* get_first_skeleton_id(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        return NULL;
    }

    // Read entire file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc((size_t)size + 1);
    if (fread(content, 1, (size_t)size, f) != (size_t)size) { fclose(f); return NULL; }
    content[size] = '\0';
    fclose(f);

    // Find first <standard_skeleton tag
    const char *skel_start = strstr(content, "<standard_skeleton");
    if (!skel_start) {
        free(content);
        return NULL;
    }

    // Find id attribute
    const char *id_start = strstr(skel_start, "id=\"");
    if (!id_start) {
        free(content);
        return NULL;
    }
    id_start += 4; // skip 'id="'

    // Find end quote
    const char *id_end = strchr(id_start, '"');
    if (!id_end) {
        free(content);
        return NULL;
    }

    // Copy ID to new string
    size_t id_len = (size_t)(id_end - id_start);
    char *skeleton_id = malloc(id_len + 1);
    memcpy(skeleton_id, id_start, id_len);
    skeleton_id[id_len] = '\0';

    free(content);
    return skeleton_id;
}

void free_skeleton(SkeletonDef *skel) {
    free(skel);
}

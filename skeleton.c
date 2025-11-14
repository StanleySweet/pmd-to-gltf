#include "skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
    fread(content, 1, (size_t)size, f);
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
    const char *p = skel_start;

    // Parse bone hierarchy
    parse_bones_recursive(&p, skel, -1);

    free(content);
    return skel;
}

void free_skeleton(SkeletonDef *skel) {
    free(skel);
}

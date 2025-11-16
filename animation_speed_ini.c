#include "animation_speed_ini.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void trim_line(char *s) {
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

int load_animation_speed_ini(const char *base_name, AnimationSpeedConfig *cfg, 
                             const char **anim_names, uint32_t anim_count, float *out_percents) {
    if (!cfg || !out_percents) return 0;
    
    // Initialize with defaults
    cfg->default_percent = 100.0f;
    for (uint32_t i = 0; i < anim_count; i++) {
        out_percents[i] = cfg->default_percent;
    }

    // Build INI filename
    char ini_file[512];
    snprintf(ini_file, sizeof(ini_file), "%s.ini", base_name);
    
    FILE *f = fopen(ini_file, "r");
    if (!f) return 0; // File doesn't exist
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        trim_line(line);
        
        // Skip empty lines and comments
        if (!line[0] || line[0] == '#' || line[0] == ';') continue;
        
        // Skip section headers
        if (line[0] == '[') continue;
        
        // Look for key=value pairs
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        trim_line(key);
        trim_line(val);
        
        if (!*key || !*val) continue;
        
        // Parse numeric value
        char *endptr = NULL;
        double num = strtod(val, &endptr);
        if (endptr == val) continue; // Not a valid number
        
        if (strcmp(key, "default") == 0) {
            cfg->default_percent = (float)num;
            // Update all animations with the new default
            for (uint32_t i = 0; i < anim_count; i++) {
                out_percents[i] = cfg->default_percent;
            }
            continue;
        }
        
        // Try to match animation name
        for (uint32_t i = 0; i < anim_count; i++) {
            if (anim_names[i] && strcmp(anim_names[i], key) == 0) {
                out_percents[i] = (float)num;
                break;
            }
        }
    }
    
    fclose(f);
    return 1;
}

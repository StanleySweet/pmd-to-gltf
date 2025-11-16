#ifndef ANIMATION_SPEED_INI_H
#define ANIMATION_SPEED_INI_H

#include <stdint.h>

typedef struct {
    float default_percent; // 100 means original speed
} AnimationSpeedConfig;

// Parse speed overrides from <base>.ini
// Format: lines like "default=100" or "walk=40"
// Returns 1 if file found (even if no entries), 0 if missing.
int load_animation_speed_ini(const char *base_name, AnimationSpeedConfig *cfg, 
                             const char **anim_names, uint32_t anim_count, float *out_percents);

#endif

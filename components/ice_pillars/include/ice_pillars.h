#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 135
#define SCREEN_HEIGHT 240
#define MAX_PILLARS 4
#define PILLAR_WIDTH 30
#define MIN_GAP_SIZE 80
#define MAX_GAP_SIZE 100
#define PILLAR_SPACING 80

typedef struct {
    float x;
    int top_height;
    int bottom_y;
    int bottom_height;
    int gap_size;
    bool active;
    bool passed;
} ice_pillar_t;

typedef struct {
    ice_pillar_t pillars[MAX_PILLARS];
    int active_count;
    float scroll_speed;
    uint32_t spawn_timer;
    uint32_t spawn_interval;
    float difficulty_multiplier;
} ice_pillars_context_t;

void ice_pillars_init(ice_pillars_context_t* ctx);
void ice_pillars_update(ice_pillars_context_t* ctx, float difficulty_multiplier);
void ice_pillars_spawn_pillar(ice_pillars_context_t* ctx);
bool ice_pillars_check_collision(ice_pillars_context_t* ctx, int penguin_x, int penguin_y, int penguin_width, int penguin_height);
bool ice_pillars_check_passed(ice_pillars_context_t* ctx, int penguin_x);
void ice_pillars_remove_offscreen(ice_pillars_context_t* ctx);
int ice_pillars_get_active_count(ice_pillars_context_t* ctx);
ice_pillar_t* ice_pillars_get_pillar(ice_pillars_context_t* ctx, int index);
void ice_pillars_reset(ice_pillars_context_t* ctx);

#ifdef __cplusplus
}
#endif
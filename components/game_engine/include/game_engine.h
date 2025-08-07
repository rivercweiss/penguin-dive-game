#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GAME_STATE_START,
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER,
    GAME_STATE_RESTART
} game_state_t;

typedef struct {
    game_state_t state;
    uint32_t score;
    uint32_t high_score;
    uint32_t frame_count;
    float difficulty_multiplier;
} game_context_t;

void game_engine_init(game_context_t* ctx);
void game_engine_update(game_context_t* ctx);
void game_engine_start_game(game_context_t* ctx);
void game_engine_end_game(game_context_t* ctx);
void game_engine_restart_game(game_context_t* ctx);
bool game_engine_is_collision(game_context_t* ctx, int penguin_x, int penguin_y, int penguin_width, int penguin_height,
                             int pillar_x, int pillar_y, int pillar_width, int pillar_height);
bool game_engine_is_screen_edge_collision(game_context_t* ctx, int penguin_x, int penguin_y, int penguin_width, int penguin_height);
void game_engine_update_score(game_context_t* ctx);
float game_engine_get_difficulty_multiplier(game_context_t* ctx);
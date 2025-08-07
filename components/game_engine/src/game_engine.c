#include "game_engine.h"
#include <string.h>

void game_engine_init(game_context_t* ctx) {
    if (!ctx) return;
    
    memset(ctx, 0, sizeof(game_context_t));
    ctx->state = GAME_STATE_START;
    ctx->difficulty_multiplier = 1.0f;
}

void game_engine_update(game_context_t* ctx) {
    if (!ctx) return;
    
    switch (ctx->state) {
        case GAME_STATE_PLAYING:
            ctx->frame_count++;
            game_engine_update_score(ctx);
            break;
        case GAME_STATE_START:
        case GAME_STATE_GAME_OVER:
        case GAME_STATE_RESTART:
        default:
            break;
    }
}

void game_engine_start_game(game_context_t* ctx) {
    if (!ctx) return;
    
    ctx->state = GAME_STATE_PLAYING;
    ctx->score = 0;
    ctx->frame_count = 0;
    ctx->difficulty_multiplier = 1.0f;
}

void game_engine_end_game(game_context_t* ctx) {
    if (!ctx) return;
    
    ctx->state = GAME_STATE_GAME_OVER;
    if (ctx->score > ctx->high_score) {
        ctx->high_score = ctx->score;
    }
}

void game_engine_restart_game(game_context_t* ctx) {
    if (!ctx) return;
    
    uint32_t saved_high_score = ctx->high_score;
    game_engine_init(ctx);
    ctx->high_score = saved_high_score;
    game_engine_start_game(ctx);
}

bool game_engine_is_collision(game_context_t* ctx, int penguin_x, int penguin_y, int penguin_width, int penguin_height,
                             int pillar_x, int pillar_y, int pillar_width, int pillar_height) {
    (void)ctx; // Unused parameter
    
    // Simple AABB collision detection
    bool collision_x = (penguin_x < pillar_x + pillar_width) && (penguin_x + penguin_width > pillar_x);
    bool collision_y = (penguin_y < pillar_y + pillar_height) && (penguin_y + penguin_height > pillar_y);
    
    return collision_x && collision_y;
}

bool game_engine_is_screen_edge_collision(game_context_t* ctx, int penguin_x, int penguin_y, int penguin_width, int penguin_height) {
    (void)ctx; // Unused parameter
    
    // Check if penguin hits any screen edge
    if (penguin_x < 0) return true;                    // Left edge
    if (penguin_y < 0) return true;                    // Top edge  
    if (penguin_x + penguin_width > 135) return true;  // Right edge (screen width = 135)
    if (penguin_y + penguin_height > 240) return true; // Bottom edge (screen height = 240)
    
    return false;
}

void game_engine_update_score(game_context_t* ctx) {
    if (!ctx) return;
    
    // Score increases based on time survived (frame count)
    ctx->score = ctx->frame_count / 60; // Assuming 60 FPS, score = seconds survived
    
    // Update difficulty based on score
    ctx->difficulty_multiplier = 1.0f + (ctx->score * 0.1f);
}

float game_engine_get_difficulty_multiplier(game_context_t* ctx) {
    if (!ctx) return 1.0f;
    return ctx->difficulty_multiplier;
}
#include "ice_pillars.h"
#include <string.h>
#include <stdlib.h>

#define BASE_SCROLL_SPEED 1.0f
#define BASE_SPAWN_INTERVAL 480

static uint32_t pseudo_random_seed = 12345;

// Simple pseudo-random number generator for deterministic testing
static uint32_t pseudo_random(void) {
    pseudo_random_seed = pseudo_random_seed * 1103515245 + 12345;
    return pseudo_random_seed;
}

static int get_random_gap_size(void) {
    return MIN_GAP_SIZE + (pseudo_random() % (MAX_GAP_SIZE - MIN_GAP_SIZE + 1));
}

static int get_random_gap_position(int gap_size) {
    int min_y = 20; // Leave some space at top
    int max_y = SCREEN_HEIGHT - gap_size - 20; // Leave some space at bottom
    return min_y + (pseudo_random() % (max_y - min_y + 1));
}

void ice_pillars_init(ice_pillars_context_t* ctx) {
    if (!ctx) return;
    
    memset(ctx, 0, sizeof(ice_pillars_context_t));
    ctx->scroll_speed = BASE_SCROLL_SPEED;
    ctx->spawn_interval = BASE_SPAWN_INTERVAL;
    ctx->difficulty_multiplier = 1.0f;
    pseudo_random_seed = 12345; // Reset for consistent testing
    for (int i = 0; i < MAX_PILLARS; i++) {
        ctx->pillars[i].active = false;
        ctx->pillars[i].passed = false;
        ctx->pillars[i].top_height = 0;
        ctx->pillars[i].bottom_height = 0;
        ctx->pillars[i].bottom_y = 0;
        ctx->pillars[i].x = 0.0f;
    }
}

void ice_pillars_update(ice_pillars_context_t* ctx, float difficulty_multiplier) {
    if (!ctx) return;
    
    ctx->difficulty_multiplier = difficulty_multiplier;
    ctx->scroll_speed = BASE_SCROLL_SPEED * difficulty_multiplier;
    ctx->spawn_interval = (uint32_t)(BASE_SPAWN_INTERVAL / difficulty_multiplier);
    
    // Update spawn timer
    ctx->spawn_timer++;
    
    // Spawn new pillar if needed
    if (ctx->spawn_timer >= ctx->spawn_interval && ctx->active_count < MAX_PILLARS) {
        ice_pillars_spawn_pillar(ctx);
        ctx->spawn_timer = 0;
    }
    
    // Update existing pillars
    for (int i = 0; i < MAX_PILLARS; i++) {
        if (ctx->pillars[i].active) {
            ctx->pillars[i].x -= ctx->scroll_speed;
        }
    }
    
    // Remove off-screen pillars
    ice_pillars_remove_offscreen(ctx);
}

void ice_pillars_spawn_pillar(ice_pillars_context_t* ctx) {
    if (!ctx) return;
    
    // Find inactive pillar slot
    for (int i = 0; i < MAX_PILLARS; i++) {
        if (!ctx->pillars[i].active) {
            ice_pillar_t* pillar = &ctx->pillars[i];
            
            pillar->x = SCREEN_WIDTH;
            pillar->gap_size = get_random_gap_size() - (int)(ctx->difficulty_multiplier * 5); // Smaller gaps as difficulty increases
            if (pillar->gap_size < MIN_GAP_SIZE) pillar->gap_size = MIN_GAP_SIZE;
            
            int gap_y = get_random_gap_position(pillar->gap_size);
            
            pillar->top_height = gap_y;
            pillar->bottom_y = gap_y + pillar->gap_size;
            pillar->bottom_height = SCREEN_HEIGHT - pillar->bottom_y;
            pillar->active = true;
            pillar->passed = false;
            
            ctx->active_count++;
            break;
        }
    }
}

bool ice_pillars_check_collision(ice_pillars_context_t* ctx, int penguin_x, int penguin_y, int penguin_width, int penguin_height) {
    if (!ctx) return false;
    
    for (int i = 0; i < MAX_PILLARS; i++) {
        if (!ctx->pillars[i].active) continue;
        
        ice_pillar_t* pillar = &ctx->pillars[i];
        int pillar_x = (int)pillar->x;
        
        // Check if penguin is horizontally aligned with pillar
        if (penguin_x < pillar_x + PILLAR_WIDTH && penguin_x + penguin_width > pillar_x) {
            // Check collision with top pillar
            if (penguin_y < pillar->top_height) {
                return true;
            }
            
            // Check collision with bottom pillar
            if (penguin_y + penguin_height > pillar->bottom_y) {
                return true;
            }
        }
    }
    
    return false;
}

bool ice_pillars_check_passed(ice_pillars_context_t* ctx, int penguin_x) {
    if (!ctx) return false;
    
    bool any_passed = false;
    
    for (int i = 0; i < MAX_PILLARS; i++) {
        if (!ctx->pillars[i].active || ctx->pillars[i].passed) continue;
        
        ice_pillar_t* pillar = &ctx->pillars[i];
        
        // Check if penguin has passed the pillar
        if (penguin_x > (int)pillar->x + PILLAR_WIDTH) {
            pillar->passed = true;
            any_passed = true;
        }
    }
    
    return any_passed;
}

void ice_pillars_remove_offscreen(ice_pillars_context_t* ctx) {
    if (!ctx) return;
    
    for (int i = 0; i < MAX_PILLARS; i++) {
        if (ctx->pillars[i].active && ctx->pillars[i].x < -PILLAR_WIDTH) {
            ctx->pillars[i].active = false;
            ctx->pillars[i].passed = false;
            ctx->active_count--;
        }
    }
}

int ice_pillars_get_active_count(ice_pillars_context_t* ctx) {
    if (!ctx) return 0;
    return ctx->active_count;
}

ice_pillar_t* ice_pillars_get_pillar(ice_pillars_context_t* ctx, int index) {
    if (!ctx || index < 0 || index >= MAX_PILLARS) return NULL;
    return &ctx->pillars[index];
}

void ice_pillars_reset(ice_pillars_context_t* ctx) {
    if (!ctx) return;
    
    for (int i = 0; i < MAX_PILLARS; i++) {
        ctx->pillars[i].active = false;
        ctx->pillars[i].passed = false;
        ctx->pillars[i].top_height = 0;
        ctx->pillars[i].bottom_height = 0;
        ctx->pillars[i].bottom_y = 0;
        ctx->pillars[i].x = 0.0f;
    }
    ctx->active_count = 0;
    ctx->spawn_timer = 0;
}
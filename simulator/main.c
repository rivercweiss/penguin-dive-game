#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "game_engine.h"
#include "penguin_physics.h"
#include "ice_pillars.h"
#include "display_driver.h"

#define WINDOW_WIDTH 540   // 4x scale of 135
#define WINDOW_HEIGHT 960  // 4x scale of 240
#define SCALE_FACTOR 4
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000 / TARGET_FPS)

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    bool running;
    bool button_pressed;
} simulator_context_t;

static bool init_sdl(simulator_context_t* sim_ctx) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    sim_ctx->window = SDL_CreateWindow(
        "Penguin Dive Game Simulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!sim_ctx->window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    sim_ctx->renderer = SDL_CreateRenderer(sim_ctx->window, -1, SDL_RENDERER_ACCELERATED);
    if (!sim_ctx->renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(sim_ctx->window);
        SDL_Quit();
        return false;
    }
    
    sim_ctx->texture = SDL_CreateTexture(
        sim_ctx->renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT
    );
    
    if (!sim_ctx->texture) {
        printf("Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(sim_ctx->renderer);
        SDL_DestroyWindow(sim_ctx->window);
        SDL_Quit();
        return false;
    }
    
    return true;
}

static void cleanup_sdl(simulator_context_t* sim_ctx) {
    if (sim_ctx->texture) {
        SDL_DestroyTexture(sim_ctx->texture);
    }
    if (sim_ctx->renderer) {
        SDL_DestroyRenderer(sim_ctx->renderer);
    }
    if (sim_ctx->window) {
        SDL_DestroyWindow(sim_ctx->window);
    }
    SDL_Quit();
}

static void handle_events(simulator_context_t* sim_ctx) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                sim_ctx->running = false;
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_SPACE) {
                    sim_ctx->button_pressed = true;
                }
                break;
            case SDL_KEYUP:
                if (e.key.keysym.sym == SDLK_SPACE) {
                    sim_ctx->button_pressed = false;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                sim_ctx->button_pressed = true;
                break;
            case SDL_MOUSEBUTTONUP:
                sim_ctx->button_pressed = false;
                break;
        }
    }
}

static void render_frame(simulator_context_t* sim_ctx, display_context_t* display_ctx) {
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(sim_ctx->texture, NULL, &pixels, &pitch) == 0) {
        // Copy framebuffer to SDL texture
        memcpy(pixels, display_ctx->framebuffer, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));
        SDL_UnlockTexture(sim_ctx->texture);
    }
    
    // Clear renderer
    SDL_SetRenderDrawColor(sim_ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(sim_ctx->renderer);
    
    // Render texture scaled up
    SDL_RenderCopy(sim_ctx->renderer, sim_ctx->texture, NULL, NULL);
    
    // Present
    SDL_RenderPresent(sim_ctx->renderer);
}

static void draw_game_objects(display_context_t* display_ctx, 
                             penguin_t* penguin, 
                             ice_pillars_context_t* pillars_ctx,
                             game_context_t* game_ctx) {
    // Clear screen
    display_driver_clear_screen(display_ctx, COLOR_DARK_BLUE);
    
    // Draw pillars
    for (int i = 0; i < MAX_PILLARS; i++) {
        ice_pillar_t* pillar = ice_pillars_get_pillar(pillars_ctx, i);
        if (pillar && pillar->active) {
            int pillar_x = (int)pillar->x;
            
            // Draw top pillar with border
            display_driver_draw_rectangle(display_ctx, pillar_x, 0, PILLAR_WIDTH, pillar->top_height, COLOR_ICE_BLUE);
            display_driver_draw_rectangle(display_ctx, pillar_x, 0, 2, pillar->top_height, COLOR_WHITE);
            display_driver_draw_rectangle(display_ctx, pillar_x + PILLAR_WIDTH - 2, 0, 2, pillar->top_height, COLOR_WHITE);
            
            // Draw bottom pillar with border  
            display_driver_draw_rectangle(display_ctx, pillar_x, pillar->bottom_y, PILLAR_WIDTH, pillar->bottom_height, COLOR_ICE_BLUE);
            display_driver_draw_rectangle(display_ctx, pillar_x, pillar->bottom_y, 2, pillar->bottom_height, COLOR_WHITE);
            display_driver_draw_rectangle(display_ctx, pillar_x + PILLAR_WIDTH - 2, pillar->bottom_y, 2, pillar->bottom_height, COLOR_WHITE);
        }
    }
    
    // Draw penguin with better appearance
    int penguin_x = penguin_physics_get_screen_x(penguin);
    int penguin_y = penguin_physics_get_screen_y(penguin);
    
    // Draw penguin body (black)
    display_driver_draw_rectangle(display_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT, COLOR_BLACK);
    
    // Draw penguin belly (white)
    display_driver_draw_rectangle(display_ctx, penguin_x + 2, penguin_y + 2, PENGUIN_WIDTH - 4, PENGUIN_HEIGHT - 4, COLOR_WHITE);
    
    // Draw penguin beak (orange/yellow)
    display_driver_draw_rectangle(display_ctx, penguin_x + PENGUIN_WIDTH, penguin_y + PENGUIN_HEIGHT/2 - 1, 3, 2, COLOR_YELLOW);
    
    // Draw score (simple text representation)
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "Score: %lu", (unsigned long)game_ctx->score);
    display_driver_draw_text(display_ctx, 5, 5, score_text, COLOR_WHITE);
    
    if (game_ctx->state == GAME_STATE_GAME_OVER) {
        display_driver_draw_text(display_ctx, 30, 100, "GAME OVER", COLOR_RED);
        display_driver_draw_text(display_ctx, 20, 120, "SPACE to restart", COLOR_WHITE);
    } else if (game_ctx->state == GAME_STATE_START) {
        display_driver_draw_text(display_ctx, 20, 100, "DIVING PENGUIN", COLOR_WHITE);
        display_driver_draw_text(display_ctx, 10, 120, "SPACE to start", COLOR_WHITE);
    }
    
    // Swap buffers
    display_driver_swap_buffers(display_ctx);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Starting Penguin Dive Game Simulator...\n");
    printf("Controls: SPACE key or mouse click to dive\n");
    printf("Goal: Navigate through ice pillars without collision\n\n");
    
    simulator_context_t sim_ctx = {0};
    display_context_t display_ctx = {0};
    game_context_t game_ctx = {0};
    penguin_t penguin = {0};
    ice_pillars_context_t pillars_ctx = {0};
    
    // Initialize SDL
    if (!init_sdl(&sim_ctx)) {
        return -1;
    }
    
    // Initialize game components
    if (!display_driver_init(&display_ctx)) {
        printf("Failed to initialize display driver\n");
        cleanup_sdl(&sim_ctx);
        return -1;
    }
    
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    sim_ctx.running = true;
    sim_ctx.button_pressed = false;
    
    Uint32 last_time = SDL_GetTicks();
    
    // Game loop
    while (sim_ctx.running) {
        Uint32 current_time = SDL_GetTicks();
        
        // Handle events
        handle_events(&sim_ctx);
        
        // Update game logic at target framerate
        if (current_time - last_time >= FRAME_TIME_MS) {
            // Handle state transitions
            if (game_ctx.state == GAME_STATE_START && sim_ctx.button_pressed) {
                game_engine_start_game(&game_ctx);
            } else if (game_ctx.state == GAME_STATE_GAME_OVER && sim_ctx.button_pressed) {
                game_engine_restart_game(&game_ctx);
                penguin_physics_init(&penguin);
                ice_pillars_reset(&pillars_ctx);
            }
            
            if (game_ctx.state == GAME_STATE_PLAYING) {
                // Update penguin physics
                penguin_physics_update(&penguin, sim_ctx.button_pressed);
                
                // Update pillars
                ice_pillars_update(&pillars_ctx, game_engine_get_difficulty_multiplier(&game_ctx));
                
                // Check for pillar passing
                int penguin_x = penguin_physics_get_screen_x(&penguin);
                if (ice_pillars_check_passed(&pillars_ctx, penguin_x)) {
                    printf("Pillar passed! Score: %lu\n", (unsigned long)game_ctx.score);
                }
                
                // Check for collisions
                int penguin_y = penguin_physics_get_screen_y(&penguin);
                if (ice_pillars_check_collision(&pillars_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
                    printf("Collision detected! Final score: %lu\n", (unsigned long)game_ctx.score);
                    game_engine_end_game(&game_ctx);
                }
                
                // Update game engine
                game_engine_update(&game_ctx);
            }
            
            // Render frame
            draw_game_objects(&display_ctx, &penguin, &pillars_ctx, &game_ctx);
            render_frame(&sim_ctx, &display_ctx);
            
            last_time = current_time;
        }
        
        // Small delay to prevent excessive CPU usage
        SDL_Delay(1);
    }
    
    // Cleanup
    display_driver_deinit(&display_ctx);
    cleanup_sdl(&sim_ctx);
    
    printf("Simulator shutdown complete.\n");
    return 0;
}
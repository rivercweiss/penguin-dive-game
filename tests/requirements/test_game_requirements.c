#include "unity.h"
#include "game_engine.h"
#include "penguin_physics.h"
#include "ice_pillars.h"
#include "display_driver.h"

// Tests based directly on the game requirements from README

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Requirement: "The goal of the game is to get as far as possible without the penguin contacting an ice pillar"
void test_requirement_collision_ends_game(void) {
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    game_engine_start_game(&game_ctx);
    
    // Spawn a pillar
    ice_pillars_spawn_pillar(&pillars_ctx);
    ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, 0);
    TEST_ASSERT_NOT_NULL(pillar);
    TEST_ASSERT_TRUE(pillar->active);
    
    // Position penguin to collide with pillar
    penguin.x = pillar->x;
    penguin.y = 10; // In top pillar area
    
    // Check collision
    bool collision = ice_pillars_check_collision(&pillars_ctx, 
                                                penguin_physics_get_screen_x(&penguin),
                                                penguin_physics_get_screen_y(&penguin),
                                                PENGUIN_WIDTH, PENGUIN_HEIGHT);
    
    TEST_ASSERT_TRUE(collision);
    
    // End game when collision occurs
    if (collision) {
        game_engine_end_game(&game_ctx);
    }
    
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
}

// Requirement: "When this happens, the score for how far the player made it is shown, and the game restarts"
void test_requirement_score_shown_and_game_restarts(void) {
    game_context_t game_ctx;
    
    game_engine_init(&game_ctx);
    game_engine_start_game(&game_ctx);
    
    // Simulate some gameplay time
    for (int i = 0; i < 300; i++) { // 5 seconds worth
        game_engine_update(&game_ctx);
    }
    
    uint32_t final_score = game_ctx.score;
    TEST_ASSERT_GREATER_THAN(0, final_score);
    
    // End game
    game_engine_end_game(&game_ctx);
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    TEST_ASSERT_EQUAL(final_score, game_ctx.high_score); // Score should be preserved
    
    // Restart game
    game_engine_restart_game(&game_ctx);
    TEST_ASSERT_EQUAL(GAME_STATE_PLAYING, game_ctx.state);
    TEST_ASSERT_EQUAL(0, game_ctx.score); // Score reset to 0
    TEST_ASSERT_EQUAL(final_score, game_ctx.high_score); // High score preserved
}

// Requirement: "The penguin will swim through gaps in ice pillars"
void test_requirement_penguin_swims_through_gaps(void) {
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    // Spawn a pillar
    ice_pillars_spawn_pillar(&pillars_ctx);
    ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, 0);
    TEST_ASSERT_NOT_NULL(pillar);
    
    // Position penguin in the gap
    penguin.x = pillar->x;
    penguin.y = pillar->top_height + (pillar->gap_size / 2); // Center of gap
    
    // Should not collide when in gap
    bool collision = ice_pillars_check_collision(&pillars_ctx,
                                                penguin_physics_get_screen_x(&penguin),
                                                penguin_physics_get_screen_y(&penguin),
                                                PENGUIN_WIDTH, PENGUIN_HEIGHT);
    
    TEST_ASSERT_FALSE(collision);
    
    // Verify gap exists between top and bottom pillars
    TEST_ASSERT_GREATER_THAN(pillar->top_height, 0);
    TEST_ASSERT_GREATER_THAN(pillar->gap_size, 0);
    TEST_ASSERT_EQUAL(pillar->top_height + pillar->gap_size, pillar->bottom_y);
}

// Requirement: "diving each time the button is pressed and rising otherwise"
void test_requirement_diving_and_rising_mechanics(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    float initial_y = penguin.y;
    
    // Test rising when button not pressed
    penguin_physics_update(&penguin, false);
    penguin_physics_update(&penguin, false);
    penguin_physics_update(&penguin, false);
    
    float rising_y = penguin.y;
    TEST_ASSERT_LESS_THAN(initial_y, rising_y); // Should move up (rise)
    
    // Test diving when button pressed
    penguin_physics_update(&penguin, true);
    penguin_physics_update(&penguin, true);
    penguin_physics_update(&penguin, true);
    
    float diving_y = penguin.y;
    TEST_ASSERT_GREATER_THAN(rising_y, diving_y); // Should move down (dive)
}

// Requirement: "The pillars will get harder and harder to navigate though as the game progresses"
void test_requirement_increasing_pillar_difficulty(void) {
    ice_pillars_context_t pillars_ctx;
    ice_pillars_init(&pillars_ctx);
    
    // Test base difficulty
    ice_pillars_update(&pillars_ctx, 1.0f);
    float base_speed = pillars_ctx.scroll_speed;
    uint32_t base_spawn_interval = pillars_ctx.spawn_interval;
    
    // Test increased difficulty
    ice_pillars_update(&pillars_ctx, 2.0f);
    float increased_speed = pillars_ctx.scroll_speed;
    uint32_t increased_spawn_interval = pillars_ctx.spawn_interval;
    
    // Speed should increase with difficulty
    TEST_ASSERT_GREATER_THAN(base_speed, increased_speed);
    
    // Spawn interval should decrease (spawn more frequently)
    TEST_ASSERT_LESS_THAN(base_spawn_interval, increased_spawn_interval);
    
    // Test gap size reduction with difficulty
    ice_pillars_update(&pillars_ctx, 3.0f);
    ice_pillars_spawn_pillar(&pillars_ctx);
    
    ice_pillar_t* difficult_pillar = NULL;
    for (int i = 0; i < MAX_PILLARS; i++) {
        ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, i);
        if (pillar && pillar->active && pillar->top_height > 0 && pillar->bottom_height > 0) {
            difficult_pillar = pillar;
            break;
        }
    }
    
    TEST_ASSERT_NOT_NULL(difficult_pillar);
    TEST_ASSERT_GREATER_OR_EQUAL(MIN_GAP_SIZE, difficult_pillar->gap_size);
    // Gap should be smaller than maximum due to difficulty
    TEST_ASSERT_LESS_THAN(MAX_GAP_SIZE, difficult_pillar->gap_size);
}

// Requirement: "move faster and faster"
void test_requirement_increasing_movement_speed(void) {
    ice_pillars_context_t pillars_ctx;
    ice_pillars_init(&pillars_ctx);
    
    // Test various difficulty levels
    float difficulty_1 = 1.0f;
    float difficulty_2 = 2.0f;
    float difficulty_3 = 3.0f;
    
    ice_pillars_update(&pillars_ctx, difficulty_1);
    float speed_1 = pillars_ctx.scroll_speed;
    
    ice_pillars_update(&pillars_ctx, difficulty_2);
    float speed_2 = pillars_ctx.scroll_speed;
    
    ice_pillars_update(&pillars_ctx, difficulty_3);
    float speed_3 = pillars_ctx.scroll_speed;
    
    // Speed should increase with difficulty
    TEST_ASSERT_GREATER_THAN(speed_1, speed_2);
    TEST_ASSERT_GREATER_THAN(speed_2, speed_3);
    
    // Verify proportional increase
    TEST_ASSERT_FLOAT_WITHIN(0.001, speed_1 * 2.0f, speed_2);
    TEST_ASSERT_FLOAT_WITHIN(0.001, speed_1 * 3.0f, speed_3);
}

// Requirement: "This penguin sprite stays on the left side of the screen"
void test_requirement_penguin_stays_left_side(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Initial position should be on left side
    TEST_ASSERT_LESS_THAN(SCREEN_WIDTH / 2, penguin.x);
    
    // After updates, penguin should remain on left side
    for (int i = 0; i < 100; i++) {
        penguin_physics_update(&penguin, i % 2 == 0);
    }
    
    TEST_ASSERT_LESS_THAN(SCREEN_WIDTH / 2, penguin.x);
    
    // Even when constrained, should stay on left side
    penguin_physics_constrain_to_screen(&penguin);
    TEST_ASSERT_LESS_THAN(SCREEN_WIDTH / 2, penguin.x);
}

// Requirement: "The penguins movement is smooth and physics based"
void test_requirement_smooth_physics_movement(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    float positions[10];
    
    // Record positions over several updates
    for (int i = 0; i < 10; i++) {
        penguin_physics_update(&penguin, false); // Rising
        positions[i] = penguin.y;
    }
    
    // Movement should be smooth - check that velocity changes gradually
    for (int i = 2; i < 10; i++) {
        float velocity_1 = positions[i-1] - positions[i-2];
        float velocity_2 = positions[i] - positions[i-1];
        float acceleration = velocity_2 - velocity_1;
        
        // Acceleration should be consistent (physics-based)
        TEST_ASSERT_FLOAT_WITHIN(0.1f, penguin.acceleration_y, acceleration);
    }
}

// Requirement: "These ice pillars come from both the top and the bottom of the screen"
void test_requirement_pillars_top_and_bottom(void) {
    ice_pillars_context_t pillars_ctx;
    ice_pillars_init(&pillars_ctx);
    
    ice_pillars_spawn_pillar(&pillars_ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, 0);
    TEST_ASSERT_NOT_NULL(pillar);
    TEST_ASSERT_TRUE(pillar->active);
    
    // Top pillar should start from top of screen (y=0)
    TEST_ASSERT_GREATER_THAN(0, pillar->top_height);
    
    // Bottom pillar should extend to bottom of screen
    TEST_ASSERT_EQUAL(SCREEN_HEIGHT, pillar->bottom_y + pillar->bottom_height);
    
    // There should be a gap between them
    TEST_ASSERT_GREATER_THAN(pillar->top_height, pillar->bottom_y);
    TEST_ASSERT_EQUAL(pillar->gap_size, pillar->bottom_y - pillar->top_height);
}

// Requirement: "There can be multiple sets of ice pillars on screen at one time"
void test_requirement_multiple_pillar_sets(void) {
    ice_pillars_context_t pillars_ctx;
    ice_pillars_init(&pillars_ctx);
    
    // Spawn multiple pillars
    ice_pillars_spawn_pillar(&pillars_ctx);
    ice_pillars_spawn_pillar(&pillars_ctx);
    ice_pillars_spawn_pillar(&pillars_ctx);
    
    TEST_ASSERT_EQUAL(3, ice_pillars_get_active_count(&pillars_ctx));
    
    // Verify each pillar is active and properly positioned
    int active_count = 0;
    for (int i = 0; i < MAX_PILLARS; i++) {
        ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, i);
        if (pillar && pillar->active && pillar->top_height > 0 && pillar->bottom_height > 0) {
            active_count++;
            TEST_ASSERT_GREATER_THAN(0, pillar->top_height);
            TEST_ASSERT_GREATER_THAN(0, pillar->gap_size);
            TEST_ASSERT_GREATER_THAN(0, pillar->bottom_height);
        }
    }
    
    TEST_ASSERT_EQUAL(3, active_count);
}

// Requirement: "The game should also be lost if the player makes the penguin hit the edges of the screen"
void test_requirement_screen_edge_collision_ends_game(void) {
    game_context_t game_ctx;
    penguin_t penguin;
    
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    
    game_engine_start_game(&game_ctx);
    TEST_ASSERT_EQUAL(GAME_STATE_PLAYING, game_ctx.state);
    
    // Test left edge collision
    penguin.x = -1.0f;
    penguin.y = 100.0f;
    
    bool collision = game_engine_is_screen_edge_collision(&game_ctx,
                                                        penguin_physics_get_screen_x(&penguin),
                                                        penguin_physics_get_screen_y(&penguin),
                                                        PENGUIN_WIDTH, PENGUIN_HEIGHT);
    TEST_ASSERT_TRUE(collision);
    
    if (collision) {
        game_engine_end_game(&game_ctx);
    }
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    
    // Test right edge collision
    game_engine_restart_game(&game_ctx);
    penguin_physics_init(&penguin);
    penguin.x = 136.0f; // Beyond right edge (screen width is 135)
    penguin.y = 100.0f;
    
    collision = game_engine_is_screen_edge_collision(&game_ctx,
                                                   penguin_physics_get_screen_x(&penguin),
                                                   penguin_physics_get_screen_y(&penguin),
                                                   PENGUIN_WIDTH, PENGUIN_HEIGHT);
    TEST_ASSERT_TRUE(collision);
    
    if (collision) {
        game_engine_end_game(&game_ctx);
    }
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    
    // Test top edge collision
    game_engine_restart_game(&game_ctx);
    penguin_physics_init(&penguin);
    penguin.x = 50.0f;
    penguin.y = -1.0f; // Above top edge
    
    collision = game_engine_is_screen_edge_collision(&game_ctx,
                                                   penguin_physics_get_screen_x(&penguin),
                                                   penguin_physics_get_screen_y(&penguin),
                                                   PENGUIN_WIDTH, PENGUIN_HEIGHT);
    TEST_ASSERT_TRUE(collision);
    
    if (collision) {
        game_engine_end_game(&game_ctx);
    }
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    
    // Test bottom edge collision
    game_engine_restart_game(&game_ctx);
    penguin_physics_init(&penguin);
    penguin.x = 50.0f;
    penguin.y = 241.0f; // Below bottom edge (screen height is 240)
    
    collision = game_engine_is_screen_edge_collision(&game_ctx,
                                                   penguin_physics_get_screen_x(&penguin),
                                                   penguin_physics_get_screen_y(&penguin),
                                                   PENGUIN_WIDTH, PENGUIN_HEIGHT);
    TEST_ASSERT_TRUE(collision);
    
    if (collision) {
        game_engine_end_game(&game_ctx);
    }
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
}

void app_main(void) {
    UNITY_BEGIN();
    
    // Core Game Objective Requirements
    RUN_TEST(test_requirement_collision_ends_game);
    RUN_TEST(test_requirement_score_shown_and_game_restarts);
    RUN_TEST(test_requirement_penguin_swims_through_gaps);
    
    // Penguin Movement Requirements
    RUN_TEST(test_requirement_diving_and_rising_mechanics);
    RUN_TEST(test_requirement_penguin_stays_left_side);
    RUN_TEST(test_requirement_smooth_physics_movement);
    
    // Difficulty Progression Requirements
    RUN_TEST(test_requirement_increasing_pillar_difficulty);
    RUN_TEST(test_requirement_increasing_movement_speed);
    
    // Ice Pillar Requirements
    RUN_TEST(test_requirement_pillars_top_and_bottom);
    RUN_TEST(test_requirement_multiple_pillar_sets);
    
    // Screen Edge Collision Requirements
    RUN_TEST(test_requirement_screen_edge_collision_ends_game);
    
    UNITY_END();
}
#include "unity.h"
#include "game_engine.h"

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Test Game State Management
void test_game_engine_init(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    TEST_ASSERT_EQUAL(GAME_STATE_START, ctx.state);
    TEST_ASSERT_EQUAL(0, ctx.score);
    TEST_ASSERT_EQUAL(0, ctx.high_score);
    TEST_ASSERT_EQUAL(0, ctx.frame_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0f, ctx.difficulty_multiplier);
}

void test_game_engine_start_game(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    game_engine_start_game(&ctx);
    
    TEST_ASSERT_EQUAL(GAME_STATE_PLAYING, ctx.state);
    TEST_ASSERT_EQUAL(0, ctx.score);
    TEST_ASSERT_EQUAL(0, ctx.frame_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0f, ctx.difficulty_multiplier);
}

void test_game_engine_end_game(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    game_engine_start_game(&ctx);
    
    // Simulate some gameplay
    ctx.score = 100;
    
    game_engine_end_game(&ctx);
    
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, ctx.state);
    TEST_ASSERT_EQUAL(100, ctx.high_score);
}

void test_game_engine_restart_game(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    game_engine_start_game(&ctx);
    
    // Simulate some gameplay and end game
    ctx.score = 50;
    ctx.frame_count = 3000;
    game_engine_end_game(&ctx);
    
    // Restart should preserve high score but reset everything else
    game_engine_restart_game(&ctx);
    
    TEST_ASSERT_EQUAL(GAME_STATE_PLAYING, ctx.state);
    TEST_ASSERT_EQUAL(0, ctx.score);
    TEST_ASSERT_EQUAL(0, ctx.frame_count);
    TEST_ASSERT_EQUAL(50, ctx.high_score); // High score preserved
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0f, ctx.difficulty_multiplier);
}

// Test Score Tracking
void test_game_engine_update_score(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    game_engine_start_game(&ctx);
    
    ctx.frame_count = 60; // 1 second at 60 FPS
    game_engine_update_score(&ctx);
    
    TEST_ASSERT_EQUAL(1, ctx.score);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.1f, ctx.difficulty_multiplier);
    
    ctx.frame_count = 600; // 10 seconds
    game_engine_update_score(&ctx);
    
    TEST_ASSERT_EQUAL(10, ctx.score);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 2.0f, ctx.difficulty_multiplier);
}

void test_game_engine_update_with_playing_state(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    game_engine_start_game(&ctx);
    
    uint32_t initial_frame_count = ctx.frame_count;
    
    game_engine_update(&ctx);
    
    TEST_ASSERT_EQUAL(initial_frame_count + 1, ctx.frame_count);
}

void test_game_engine_update_with_non_playing_state(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    uint32_t initial_frame_count = ctx.frame_count;
    
    game_engine_update(&ctx);
    
    TEST_ASSERT_EQUAL(initial_frame_count, ctx.frame_count); // Should not increment
}

// Test Collision Detection
void test_game_engine_collision_detection_no_collision(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin at (10, 10) with size 20x20
    // Pillar at (50, 50) with size 30x30
    bool collision = game_engine_is_collision(&ctx, 10, 10, 20, 20, 50, 50, 30, 30);
    
    TEST_ASSERT_FALSE(collision);
}

void test_game_engine_collision_detection_with_collision(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin at (10, 10) with size 20x20
    // Pillar at (20, 20) with size 30x30 (overlapping)
    bool collision = game_engine_is_collision(&ctx, 10, 10, 20, 20, 20, 20, 30, 30);
    
    TEST_ASSERT_TRUE(collision);
}

void test_game_engine_collision_detection_edge_case(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin at (10, 10) with size 10x10
    // Pillar at (20, 20) with size 10x10 (touching edges, no overlap)
    bool collision = game_engine_is_collision(&ctx, 10, 10, 10, 10, 20, 20, 10, 10);
    
    TEST_ASSERT_FALSE(collision);
}

// Test Screen Edge Collision Detection
void test_game_engine_screen_edge_collision_left(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin partially off left edge
    bool collision = game_engine_is_screen_edge_collision(&ctx, -5, 50, 20, 20);
    TEST_ASSERT_TRUE(collision);
    
    // Penguin just touching left edge (should not collide)
    collision = game_engine_is_screen_edge_collision(&ctx, 0, 50, 20, 20);
    TEST_ASSERT_FALSE(collision);
}

void test_game_engine_screen_edge_collision_right(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin partially off right edge (135 is screen width)
    bool collision = game_engine_is_screen_edge_collision(&ctx, 130, 50, 20, 20);
    TEST_ASSERT_TRUE(collision);
    
    // Penguin just touching right edge (should not collide)
    collision = game_engine_is_screen_edge_collision(&ctx, 115, 50, 20, 20);
    TEST_ASSERT_FALSE(collision);
}

void test_game_engine_screen_edge_collision_top(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin partially off top edge
    bool collision = game_engine_is_screen_edge_collision(&ctx, 50, -5, 20, 20);
    TEST_ASSERT_TRUE(collision);
    
    // Penguin just touching top edge (should not collide)
    collision = game_engine_is_screen_edge_collision(&ctx, 50, 0, 20, 20);
    TEST_ASSERT_FALSE(collision);
}

void test_game_engine_screen_edge_collision_bottom(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin partially off bottom edge (240 is screen height)
    bool collision = game_engine_is_screen_edge_collision(&ctx, 50, 235, 20, 20);
    TEST_ASSERT_TRUE(collision);
    
    // Penguin just touching bottom edge (should not collide)
    collision = game_engine_is_screen_edge_collision(&ctx, 50, 220, 20, 20);
    TEST_ASSERT_FALSE(collision);
}

void test_game_engine_screen_edge_collision_no_collision_center(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin in center of screen
    bool collision = game_engine_is_screen_edge_collision(&ctx, 50, 100, 20, 20);
    TEST_ASSERT_FALSE(collision);
}

void test_game_engine_screen_edge_collision_corner_cases(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    // Penguin in corners (should collide)
    bool collision = game_engine_is_screen_edge_collision(&ctx, -1, -1, 20, 20); // Top-left corner
    TEST_ASSERT_TRUE(collision);
    
    collision = game_engine_is_screen_edge_collision(&ctx, 130, -1, 20, 20); // Top-right corner
    TEST_ASSERT_TRUE(collision);
    
    collision = game_engine_is_screen_edge_collision(&ctx, -1, 235, 20, 20); // Bottom-left corner
    TEST_ASSERT_TRUE(collision);
    
    collision = game_engine_is_screen_edge_collision(&ctx, 130, 235, 20, 20); // Bottom-right corner
    TEST_ASSERT_TRUE(collision);
}

// Test Game Progression (Difficulty)
void test_game_engine_difficulty_progression(void) {
    game_context_t ctx;
    game_engine_init(&ctx);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0f, game_engine_get_difficulty_multiplier(&ctx));
    
    // Simulate 10 seconds of gameplay
    ctx.frame_count = 600;
    game_engine_update_score(&ctx);
    
    float difficulty = game_engine_get_difficulty_multiplier(&ctx);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 2.0f, difficulty);
}

void app_main(void) {
    UNITY_BEGIN();
    
    // Game State Management Tests
    RUN_TEST(test_game_engine_init);
    RUN_TEST(test_game_engine_start_game);
    RUN_TEST(test_game_engine_end_game);
    RUN_TEST(test_game_engine_restart_game);
    
    // Score Tracking Tests
    RUN_TEST(test_game_engine_update_score);
    RUN_TEST(test_game_engine_update_with_playing_state);
    RUN_TEST(test_game_engine_update_with_non_playing_state);
    
    // Collision Detection Tests
    RUN_TEST(test_game_engine_collision_detection_no_collision);
    RUN_TEST(test_game_engine_collision_detection_with_collision);
    RUN_TEST(test_game_engine_collision_detection_edge_case);
    
    // Screen Edge Collision Tests
    RUN_TEST(test_game_engine_screen_edge_collision_left);
    RUN_TEST(test_game_engine_screen_edge_collision_right);
    RUN_TEST(test_game_engine_screen_edge_collision_top);
    RUN_TEST(test_game_engine_screen_edge_collision_bottom);
    RUN_TEST(test_game_engine_screen_edge_collision_no_collision_center);
    RUN_TEST(test_game_engine_screen_edge_collision_corner_cases);
    
    // Game Progression Tests
    RUN_TEST(test_game_engine_difficulty_progression);
    
    UNITY_END();
}
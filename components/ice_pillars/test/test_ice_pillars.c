#include "unity.h"
#include "ice_pillars.h"

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Test Pillar Generation - Top/bottom pillars with gaps
void test_ice_pillars_init(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    TEST_ASSERT_EQUAL(0, ctx.active_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 2.0f, ctx.scroll_speed);
    TEST_ASSERT_EQUAL(120, ctx.spawn_interval);
    TEST_ASSERT_EQUAL(0, ctx.spawn_timer);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0f, ctx.difficulty_multiplier);
    
    // All pillars should be inactive
    for (int i = 0; i < MAX_PILLARS; i++) {
        TEST_ASSERT_FALSE(ctx.pillars[i].active);
    }
}

void test_ice_pillars_spawn_pillar(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    TEST_ASSERT_EQUAL(1, ctx.active_count);
    
    // Find the spawned pillar
    ice_pillar_t* pillar = NULL;
    for (int i = 0; i < MAX_PILLARS; i++) {
        if (ctx.pillars[i].active) {
            pillar = &ctx.pillars[i];
            break;
        }
    }
    
    TEST_ASSERT_NOT_NULL(pillar);
    TEST_ASSERT_FLOAT_WITHIN(0.001, SCREEN_WIDTH, pillar->x);
    TEST_ASSERT_GREATER_OR_EQUAL(MIN_GAP_SIZE, pillar->gap_size);
    TEST_ASSERT_LESS_OR_EQUAL(MAX_GAP_SIZE, pillar->gap_size);
    TEST_ASSERT_GREATER_THAN(0, pillar->top_height);
    TEST_ASSERT_GREATER_THAN(pillar->top_height, pillar->bottom_y);
    TEST_ASSERT_GREATER_THAN(0, pillar->bottom_height);
    TEST_ASSERT_FALSE(pillar->passed);
}

void test_ice_pillars_spawn_multiple_pillars(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    // Spawn maximum number of pillars
    for (int i = 0; i < MAX_PILLARS; i++) {
        ice_pillars_spawn_pillar(&ctx);
    }
    
    TEST_ASSERT_EQUAL(MAX_PILLARS, ctx.active_count);
    
    // Try to spawn one more - should not increase count
    ice_pillars_spawn_pillar(&ctx);
    TEST_ASSERT_EQUAL(MAX_PILLARS, ctx.active_count);
}

void test_ice_pillars_gap_properties(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    TEST_ASSERT_NOT_NULL(pillar);
    
    if (pillar->active) {
        // Gap size should be within valid range
        TEST_ASSERT_GREATER_OR_EQUAL(MIN_GAP_SIZE, pillar->gap_size);
        TEST_ASSERT_LESS_OR_EQUAL(MAX_GAP_SIZE, pillar->gap_size);
        
        // Gap should be properly positioned
        int calculated_gap = pillar->bottom_y - pillar->top_height;
        TEST_ASSERT_EQUAL(pillar->gap_size, calculated_gap);
        
        // Bottom pillar should reach screen bottom
        TEST_ASSERT_EQUAL(SCREEN_HEIGHT - pillar->bottom_y, pillar->bottom_height);
    }
}

// Test Multiple Pillars - Multiple sets on screen simultaneously
void test_ice_pillars_multiple_active_pillars(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    // Spawn 3 pillars
    for (int i = 0; i < 3; i++) {
        ice_pillars_spawn_pillar(&ctx);
    }
    
    TEST_ASSERT_EQUAL(3, ctx.active_count);
    
    int active_found = 0;
    for (int i = 0; i < MAX_PILLARS; i++) {
        if (ctx.pillars[i].active) {
            active_found++;
        }
    }
    
    TEST_ASSERT_EQUAL(3, active_found);
}

// Test Pillar Movement - Left-to-right scrolling
void test_ice_pillars_scroll_movement(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    TEST_ASSERT_NOT_NULL(pillar);
    
    if (pillar->active) {
        float initial_x = pillar->x;
        
        ice_pillars_update(&ctx, 1.0f);
        
        TEST_ASSERT_LESS_THAN(initial_x, pillar->x);
        TEST_ASSERT_FLOAT_WITHIN(0.001, initial_x - 2.0f, pillar->x);
    }
}

void test_ice_pillars_remove_offscreen(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    TEST_ASSERT_NOT_NULL(pillar);
    
    if (pillar->active) {
        // Move pillar off-screen
        pillar->x = -PILLAR_WIDTH - 1;
        
        ice_pillars_remove_offscreen(&ctx);
        
        TEST_ASSERT_FALSE(pillar->active);
        TEST_ASSERT_EQUAL(0, ctx.active_count);
    }
}

// Test Difficulty Progression - Speed increase and gap size reduction
void test_ice_pillars_difficulty_speed_increase(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    // Test base difficulty
    ice_pillars_update(&ctx, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 2.0f, ctx.scroll_speed);
    
    // Test increased difficulty
    ice_pillars_update(&ctx, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 4.0f, ctx.scroll_speed);
    
    // Test very high difficulty
    ice_pillars_update(&ctx, 3.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 6.0f, ctx.scroll_speed);
}

void test_ice_pillars_difficulty_spawn_rate_increase(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    // Test base difficulty spawn interval
    ice_pillars_update(&ctx, 1.0f);
    TEST_ASSERT_EQUAL(120, ctx.spawn_interval);
    
    // Test increased difficulty spawn interval
    ice_pillars_update(&ctx, 2.0f);
    TEST_ASSERT_EQUAL(60, ctx.spawn_interval);
    
    // Test very high difficulty spawn interval
    ice_pillars_update(&ctx, 4.0f);
    TEST_ASSERT_EQUAL(30, ctx.spawn_interval);
}

void test_ice_pillars_difficulty_gap_size_reduction(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    // Test with higher difficulty
    ice_pillars_update(&ctx, 3.0f);
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    TEST_ASSERT_NOT_NULL(pillar);
    
    if (pillar->active) {
        // Gap size should be reduced but not below minimum
        TEST_ASSERT_GREATER_OR_EQUAL(MIN_GAP_SIZE, pillar->gap_size);
        // Should be smaller than max due to difficulty
        TEST_ASSERT_LESS_OR_EQUAL(MAX_GAP_SIZE - 10, pillar->gap_size);
    }
}

// Test Collision Detection
void test_ice_pillars_collision_detection_no_collision(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    // Penguin far away from pillar
    bool collision = ice_pillars_check_collision(&ctx, 10, 100, 20, 20);
    TEST_ASSERT_FALSE(collision);
}

void test_ice_pillars_collision_detection_top_pillar_collision(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    if (pillar && pillar->active && pillar->top_height > 0 && pillar->bottom_height > 0) {
        // Position penguin to collide with top pillar
        int pillar_x = (int)pillar->x;
        bool collision = ice_pillars_check_collision(&ctx, pillar_x, 0, 20, 20);
        TEST_ASSERT_TRUE(collision);
    }
}

void test_ice_pillars_collision_detection_bottom_pillar_collision(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    if (pillar && pillar->active && pillar->top_height > 0 && pillar->bottom_height > 0) {
        // Position penguin to collide with bottom pillar
        int pillar_x = (int)pillar->x;
        bool collision = ice_pillars_check_collision(&ctx, pillar_x, pillar->bottom_y, 20, 20);
        TEST_ASSERT_TRUE(collision);
    }
}

void test_ice_pillars_collision_detection_gap_no_collision(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    if (pillar && pillar->active && pillar->top_height > 0 && pillar->bottom_height > 0) {
        // Position penguin in the gap
        int pillar_x = (int)pillar->x;
        int gap_middle = pillar->top_height + (pillar->gap_size / 2);
        bool collision = ice_pillars_check_collision(&ctx, pillar_x, gap_middle, 10, 10);
        TEST_ASSERT_FALSE(collision);
    }
}

// Test Pillar Passing
void test_ice_pillars_check_passed(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    ice_pillars_spawn_pillar(&ctx);
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&ctx, 0);
    if (pillar && pillar->active && pillar->top_height > 0 && pillar->bottom_height > 0) {
        TEST_ASSERT_FALSE(pillar->passed);
        
        // Penguin hasn't passed yet
        int pillar_x = (int)pillar->x;
        bool passed = ice_pillars_check_passed(&ctx, pillar_x - 10);
        TEST_ASSERT_FALSE(passed);
        TEST_ASSERT_FALSE(pillar->passed);
        
        // Penguin passes pillar
        passed = ice_pillars_check_passed(&ctx, pillar_x + PILLAR_WIDTH + 1);
        TEST_ASSERT_TRUE(passed);
        TEST_ASSERT_TRUE(pillar->passed);
    }
}

// Test Reset Functionality
void test_ice_pillars_reset(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    // Spawn some pillars
    ice_pillars_spawn_pillar(&ctx);
    ice_pillars_spawn_pillar(&ctx);
    
    TEST_ASSERT_EQUAL(2, ctx.active_count);
    
    ice_pillars_reset(&ctx);
    
    TEST_ASSERT_EQUAL(0, ctx.active_count);
    TEST_ASSERT_EQUAL(0, ctx.spawn_timer);
    
    for (int i = 0; i < MAX_PILLARS; i++) {
        TEST_ASSERT_FALSE(ctx.pillars[i].active);
        TEST_ASSERT_FALSE(ctx.pillars[i].passed);
    }
}

// Test Update with Spawning
void test_ice_pillars_update_spawning(void) {
    ice_pillars_context_t ctx;
    ice_pillars_init(&ctx);
    
    // Update until spawn timer reaches spawn interval
    for (uint32_t i = 0; i < ctx.spawn_interval; i++) {
        ice_pillars_update(&ctx, 1.0f);
    }
    
    TEST_ASSERT_EQUAL(1, ctx.active_count);
    TEST_ASSERT_EQUAL(0, ctx.spawn_timer); // Should reset after spawn
}

void app_main(void) {
    UNITY_BEGIN();
    
    // Pillar Generation Tests
    RUN_TEST(test_ice_pillars_init);
    RUN_TEST(test_ice_pillars_spawn_pillar);
    RUN_TEST(test_ice_pillars_spawn_multiple_pillars);
    RUN_TEST(test_ice_pillars_gap_properties);
    
    // Multiple Pillars Tests
    RUN_TEST(test_ice_pillars_multiple_active_pillars);
    
    // Pillar Movement Tests
    RUN_TEST(test_ice_pillars_scroll_movement);
    RUN_TEST(test_ice_pillars_remove_offscreen);
    
    // Difficulty Progression Tests
    RUN_TEST(test_ice_pillars_difficulty_speed_increase);
    RUN_TEST(test_ice_pillars_difficulty_spawn_rate_increase);
    RUN_TEST(test_ice_pillars_difficulty_gap_size_reduction);
    
    // Collision Detection Tests
    RUN_TEST(test_ice_pillars_collision_detection_no_collision);
    RUN_TEST(test_ice_pillars_collision_detection_top_pillar_collision);
    RUN_TEST(test_ice_pillars_collision_detection_bottom_pillar_collision);
    RUN_TEST(test_ice_pillars_collision_detection_gap_no_collision);
    
    // Pillar Passing Tests
    RUN_TEST(test_ice_pillars_check_passed);
    
    // Reset Tests
    RUN_TEST(test_ice_pillars_reset);
    
    // Update Tests
    RUN_TEST(test_ice_pillars_update_spawning);
    
    UNITY_END();
}
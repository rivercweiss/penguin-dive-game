#include "unity.h"
#include "display_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Test display context initialization
void test_display_driver_init_valid_context(void) {
    display_context_t ctx = {0};
    
    bool result = display_driver_init(&ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ctx.initialized);
    TEST_ASSERT_NOT_NULL(ctx.front_buffer);
    TEST_ASSERT_NOT_NULL(ctx.back_buffer);
    TEST_ASSERT_NOT_NULL(ctx.current_buffer);
    
    display_driver_deinit(&ctx);
}

void test_display_driver_init_null_context(void) {
    bool result = display_driver_init(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_display_driver_deinit(void) {
    display_context_t ctx = {0};
    
    display_driver_init(&ctx);
    display_driver_deinit(&ctx);
    
    TEST_ASSERT_FALSE(ctx.initialized);
    TEST_ASSERT_NULL(ctx.front_buffer);
    TEST_ASSERT_NULL(ctx.back_buffer);
}

// Test screen clearing
void test_display_driver_clear_screen(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    // Clear to white
    display_driver_clear_screen(&ctx, COLOR_WHITE);
    
    // Check that buffer is filled with white pixels
    uint32_t pixel_count = DISPLAY_WIDTH * DISPLAY_HEIGHT;
    for (uint32_t i = 0; i < pixel_count; i++) {
        TEST_ASSERT_EQUAL(COLOR_WHITE, ctx.current_buffer[i]);
    }
    
    // Clear to black
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    // Check that buffer is filled with black pixels
    for (uint32_t i = 0; i < pixel_count; i++) {
        TEST_ASSERT_EQUAL(COLOR_BLACK, ctx.current_buffer[i]);
    }
    
    display_driver_deinit(&ctx);
}

void test_display_driver_clear_screen_null_context(void) {
    // Should not crash
    display_driver_clear_screen(NULL, COLOR_WHITE);
    TEST_ASSERT_TRUE(true); // If we reach here, it didn't crash
}

// Test rectangle drawing
void test_display_driver_draw_rectangle_basic(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    // Clear screen to black first
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    // Draw a 10x10 white rectangle at (5, 5)
    display_driver_draw_rectangle(&ctx, 5, 5, 10, 10, COLOR_WHITE);
    
    // Check pixels inside rectangle are white
    for (int y = 5; y < 15; y++) {
        for (int x = 5; x < 15; x++) {
            TEST_ASSERT_EQUAL(COLOR_WHITE, ctx.current_buffer[y * DISPLAY_WIDTH + x]);
        }
    }
    
    // Check pixels outside rectangle are still black
    TEST_ASSERT_EQUAL(COLOR_BLACK, ctx.current_buffer[4 * DISPLAY_WIDTH + 4]); // Top-left outside
    TEST_ASSERT_EQUAL(COLOR_BLACK, ctx.current_buffer[15 * DISPLAY_WIDTH + 15]); // Bottom-right outside
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_rectangle_clipping(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    // Draw rectangle that extends beyond screen boundaries
    display_driver_draw_rectangle(&ctx, DISPLAY_WIDTH - 5, DISPLAY_HEIGHT - 5, 10, 10, COLOR_RED);
    
    // Check that only the visible part was drawn
    for (int y = DISPLAY_HEIGHT - 5; y < DISPLAY_HEIGHT; y++) {
        for (int x = DISPLAY_WIDTH - 5; x < DISPLAY_WIDTH; x++) {
            TEST_ASSERT_EQUAL(COLOR_RED, ctx.current_buffer[y * DISPLAY_WIDTH + x]);
        }
    }
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_rectangle_completely_outside(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    // Draw rectangle completely outside screen
    display_driver_draw_rectangle(&ctx, DISPLAY_WIDTH + 10, DISPLAY_HEIGHT + 10, 10, 10, COLOR_RED);
    
    // Check that screen is still all black
    uint32_t pixel_count = DISPLAY_WIDTH * DISPLAY_HEIGHT;
    for (uint32_t i = 0; i < pixel_count; i++) {
        TEST_ASSERT_EQUAL(COLOR_BLACK, ctx.current_buffer[i]);
    }
    
    display_driver_deinit(&ctx);
}

// Test text drawing
void test_display_driver_draw_text_basic(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    display_driver_draw_text(&ctx, 0, 0, "A", COLOR_WHITE);
    
    // Check that some pixels are now white (the 'A' character)
    bool found_white = false;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (ctx.current_buffer[y * DISPLAY_WIDTH + x] == COLOR_WHITE) {
                found_white = true;
                break;
            }
        }
        if (found_white) break;
    }
    TEST_ASSERT_TRUE(found_white);
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_text_multichar(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    display_driver_draw_text(&ctx, 0, 0, "ABC", COLOR_WHITE);
    
    // Check that pixels in three character positions have some white
    bool found_white_char1 = false, found_white_char2 = false, found_white_char3 = false;
    
    // Character 1 (A) at (0,0)
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (ctx.current_buffer[y * DISPLAY_WIDTH + x] == COLOR_WHITE) {
                found_white_char1 = true;
                break;
            }
        }
        if (found_white_char1) break;
    }
    
    // Character 2 (B) at (8,0)
    for (int y = 0; y < 8; y++) {
        for (int x = 8; x < 16; x++) {
            if (ctx.current_buffer[y * DISPLAY_WIDTH + x] == COLOR_WHITE) {
                found_white_char2 = true;
                break;
            }
        }
        if (found_white_char2) break;
    }
    
    // Character 3 (C) at (16,0)
    for (int y = 0; y < 8; y++) {
        for (int x = 16; x < 24; x++) {
            if (ctx.current_buffer[y * DISPLAY_WIDTH + x] == COLOR_WHITE) {
                found_white_char3 = true;
                break;
            }
        }
        if (found_white_char3) break;
    }
    
    TEST_ASSERT_TRUE(found_white_char1);
    TEST_ASSERT_TRUE(found_white_char2);
    TEST_ASSERT_TRUE(found_white_char3);
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_text_null_text(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    // Should not crash
    display_driver_draw_text(&ctx, 0, 0, NULL, COLOR_WHITE);
    TEST_ASSERT_TRUE(true);
    
    display_driver_deinit(&ctx);
}

// Test buffer swapping
void test_display_driver_swap_buffers(void) {
    display_context_t ctx = {0};
    display_driver_init(&ctx);
    
    uint16_t *original_back = ctx.back_buffer;
    uint16_t *original_front = ctx.front_buffer;
    
    // Draw something in back buffer
    display_driver_clear_screen(&ctx, COLOR_RED);
    
    // Swap buffers
    display_driver_swap_buffers(&ctx);
    
    // Check that buffers were swapped
    TEST_ASSERT_EQUAL(original_back, ctx.front_buffer);
    TEST_ASSERT_EQUAL(original_front, ctx.back_buffer);
    TEST_ASSERT_EQUAL(ctx.back_buffer, ctx.current_buffer);
    
    display_driver_deinit(&ctx);
}

// Test color constants
void test_color_constants(void) {
    // Test that color constants are defined and have reasonable values
    TEST_ASSERT_EQUAL(0x0000, COLOR_BLACK);
    TEST_ASSERT_EQUAL(0xFFFF, COLOR_WHITE);
    TEST_ASSERT_EQUAL(0xF800, COLOR_RED);
    TEST_ASSERT_EQUAL(0x07E0, COLOR_GREEN);
    TEST_ASSERT_EQUAL(0x001F, COLOR_BLUE);
    TEST_ASSERT_EQUAL(0xFFE0, COLOR_YELLOW);
}

// Test display dimensions
void test_display_dimensions(void) {
    TEST_ASSERT_EQUAL(135, DISPLAY_WIDTH);
    TEST_ASSERT_EQUAL(240, DISPLAY_HEIGHT);
}

// Test invalid operations with uninitialized context
void test_operations_with_uninitialized_context(void) {
    display_context_t ctx = {0}; // Not initialized
    
    // These should not crash or cause issues
    display_driver_clear_screen(&ctx, COLOR_WHITE);
    display_driver_draw_rectangle(&ctx, 0, 0, 10, 10, COLOR_RED);
    display_driver_draw_text(&ctx, 0, 0, "Test", COLOR_WHITE);
    display_driver_swap_buffers(&ctx);
    display_driver_flush(&ctx);
    
    TEST_ASSERT_TRUE(true); // If we reach here, nothing crashed
}

void app_main(void) {
    UNITY_BEGIN();
    
    // Basic initialization tests
    RUN_TEST(test_display_driver_init_valid_context);
    RUN_TEST(test_display_driver_init_null_context);
    RUN_TEST(test_display_driver_deinit);
    
    // Screen clearing tests
    RUN_TEST(test_display_driver_clear_screen);
    RUN_TEST(test_display_driver_clear_screen_null_context);
    
    // Rectangle drawing tests
    RUN_TEST(test_display_driver_draw_rectangle_basic);
    RUN_TEST(test_display_driver_draw_rectangle_clipping);
    RUN_TEST(test_display_driver_draw_rectangle_completely_outside);
    
    // Text drawing tests
    RUN_TEST(test_display_driver_draw_text_basic);
    RUN_TEST(test_display_driver_draw_text_multichar);
    RUN_TEST(test_display_driver_draw_text_null_text);
    
    // Buffer management tests
    RUN_TEST(test_display_driver_swap_buffers);
    
    // Constants and configuration tests
    RUN_TEST(test_color_constants);
    RUN_TEST(test_display_dimensions);
    
    // Error handling tests
    RUN_TEST(test_operations_with_uninitialized_context);
    
    UNITY_END();
}
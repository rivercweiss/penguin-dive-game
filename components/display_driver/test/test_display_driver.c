#include "unity.h"
#include "display_driver.h"

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Test Display Initialization
void test_display_driver_init(void) {
    display_context_t ctx;
    
    bool result = display_driver_init(&ctx);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ctx.initialized);
    TEST_ASSERT_EQUAL(DISPLAY_WIDTH, ctx.width);
    TEST_ASSERT_EQUAL(DISPLAY_HEIGHT, ctx.height);
    TEST_ASSERT_NOT_NULL(ctx.framebuffer);
    TEST_ASSERT_NOT_NULL(ctx.back_buffer);
    TEST_ASSERT_TRUE(ctx.double_buffered);
    
    display_driver_deinit(&ctx);
}

void test_display_driver_init_null_context(void) {
    bool result = display_driver_init(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_display_driver_deinit(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    
    display_driver_deinit(&ctx);
    
    TEST_ASSERT_FALSE(ctx.initialized);
    TEST_ASSERT_NULL(ctx.framebuffer);
    TEST_ASSERT_NULL(ctx.back_buffer);
}

void test_display_driver_is_initialized(void) {
    display_context_t ctx;
    
    TEST_ASSERT_FALSE(display_driver_is_initialized(&ctx));
    
    display_driver_init(&ctx);
    TEST_ASSERT_TRUE(display_driver_is_initialized(&ctx));
    
    display_driver_deinit(&ctx);
    TEST_ASSERT_FALSE(display_driver_is_initialized(&ctx));
}

// Test Screen Clearing
void test_display_driver_clear_screen(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    
    display_driver_clear_screen(&ctx, COLOR_RED);
    
    // Check that all pixels are red
    for (int y = 0; y < 10; y++) { // Check sample of pixels
        for (int x = 0; x < 10; x++) {
            uint16_t pixel = display_driver_get_pixel(&ctx, x, y);
            TEST_ASSERT_EQUAL(COLOR_RED, pixel);
        }
    }
    
    display_driver_deinit(&ctx);
}

void test_display_driver_clear_screen_different_colors(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    
    // Test multiple colors
    display_driver_clear_screen(&ctx, COLOR_BLUE);
    TEST_ASSERT_EQUAL(COLOR_BLUE, display_driver_get_pixel(&ctx, 50, 50));
    
    display_driver_clear_screen(&ctx, COLOR_GREEN);
    TEST_ASSERT_EQUAL(COLOR_GREEN, display_driver_get_pixel(&ctx, 50, 50));
    
    display_driver_deinit(&ctx);
}

// Test Rectangle Drawing
void test_display_driver_draw_rectangle(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    display_driver_draw_rectangle(&ctx, 10, 20, 30, 40, COLOR_WHITE);
    
    // Check rectangle area
    TEST_ASSERT_EQUAL(COLOR_WHITE, display_driver_get_pixel(&ctx, 10, 20)); // Top-left
    TEST_ASSERT_EQUAL(COLOR_WHITE, display_driver_get_pixel(&ctx, 39, 59)); // Bottom-right
    TEST_ASSERT_EQUAL(COLOR_WHITE, display_driver_get_pixel(&ctx, 25, 35)); // Center
    
    // Check outside rectangle
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 9, 20));   // Left edge
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 10, 19));  // Top edge
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 40, 59));  // Right edge
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 39, 60));  // Bottom edge
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_rectangle_clipping(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    // Draw rectangle that extends beyond screen bounds
    display_driver_draw_rectangle(&ctx, -10, -10, 50, 50, COLOR_RED);
    
    // Should be clipped to (0,0) with size 40x40
    TEST_ASSERT_EQUAL(COLOR_RED, display_driver_get_pixel(&ctx, 0, 0));
    TEST_ASSERT_EQUAL(COLOR_RED, display_driver_get_pixel(&ctx, 39, 39));
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 40, 40));
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_rectangle_out_of_bounds(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    // Draw rectangle completely out of bounds
    display_driver_draw_rectangle(&ctx, 200, 200, 50, 50, COLOR_RED);
    
    // Screen should still be black
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 0, 0));
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 100, 100));
    
    display_driver_deinit(&ctx);
}

// Test Sprite Drawing
void test_display_driver_draw_sprite(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    sprite_t penguin_sprite = {
        .x = 20,
        .y = 30,
        .width = 15,
        .height = 15,
        .color = COLOR_YELLOW
    };
    
    display_driver_draw_sprite(&ctx, &penguin_sprite);
    
    // Check sprite area
    TEST_ASSERT_EQUAL(COLOR_YELLOW, display_driver_get_pixel(&ctx, 20, 30));
    TEST_ASSERT_EQUAL(COLOR_YELLOW, display_driver_get_pixel(&ctx, 34, 44));
    TEST_ASSERT_EQUAL(COLOR_YELLOW, display_driver_get_pixel(&ctx, 27, 37));
    
    // Check outside sprite
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 19, 30));
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 35, 44));
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_sprite_null(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    // Should not crash with null sprite
    display_driver_draw_sprite(&ctx, NULL);
    
    // Screen should still be black
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 50, 50));
    
    display_driver_deinit(&ctx);
}

// Test Text Drawing
void test_display_driver_draw_text(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    display_driver_draw_text(&ctx, 10, 20, "Hi", COLOR_WHITE);
    
    // Check that text area has white pixels (character blocks)
    TEST_ASSERT_EQUAL(COLOR_WHITE, display_driver_get_pixel(&ctx, 10, 20)); // First char
    TEST_ASSERT_EQUAL(COLOR_WHITE, display_driver_get_pixel(&ctx, 18, 20)); // Second char
    
    // Check outside text area
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 26, 20)); // After text
    
    display_driver_deinit(&ctx);
}

void test_display_driver_draw_text_with_newline(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    display_driver_draw_text(&ctx, 10, 20, "Hi\nBye", COLOR_WHITE);
    
    // Check first line
    TEST_ASSERT_EQUAL(COLOR_WHITE, display_driver_get_pixel(&ctx, 10, 20));
    
    // Check second line (should be 8 pixels down)
    TEST_ASSERT_EQUAL(COLOR_WHITE, display_driver_get_pixel(&ctx, 10, 28));
    
    display_driver_deinit(&ctx);
}

// Test Pixel Operations
void test_display_driver_set_get_pixel(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    
    display_driver_set_pixel(&ctx, 50, 60, COLOR_CYAN);
    
    TEST_ASSERT_EQUAL(COLOR_CYAN, display_driver_get_pixel(&ctx, 50, 60));
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 51, 60));
    
    display_driver_deinit(&ctx);
}

void test_display_driver_pixel_bounds_checking(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    
    // Out of bounds get should return 0
    TEST_ASSERT_EQUAL(0, display_driver_get_pixel(&ctx, -1, 0));
    TEST_ASSERT_EQUAL(0, display_driver_get_pixel(&ctx, 0, -1));
    TEST_ASSERT_EQUAL(0, display_driver_get_pixel(&ctx, DISPLAY_WIDTH, 0));
    TEST_ASSERT_EQUAL(0, display_driver_get_pixel(&ctx, 0, DISPLAY_HEIGHT));
    
    // Out of bounds set should not crash (test by setting then checking nearby pixel)
    display_driver_clear_screen(&ctx, COLOR_BLACK);
    display_driver_set_pixel(&ctx, -1, 0, COLOR_RED);
    display_driver_set_pixel(&ctx, 0, -1, COLOR_RED);
    display_driver_set_pixel(&ctx, DISPLAY_WIDTH, 0, COLOR_RED);
    display_driver_set_pixel(&ctx, 0, DISPLAY_HEIGHT, COLOR_RED);
    
    // Nearby valid pixels should still be black
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 0, 0));
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, DISPLAY_WIDTH - 1, 0));
    TEST_ASSERT_EQUAL(COLOR_BLACK, display_driver_get_pixel(&ctx, 0, DISPLAY_HEIGHT - 1));
    
    display_driver_deinit(&ctx);
}

// Test Buffer Swapping
void test_display_driver_swap_buffers(void) {
    display_context_t ctx;
    display_driver_init(&ctx);
    
    // Draw to back buffer
    display_driver_clear_screen(&ctx, COLOR_RED);
    display_driver_set_pixel(&ctx, 10, 10, COLOR_BLUE);
    
    // Store buffer pointers
    uint16_t* original_front = ctx.framebuffer;
    uint16_t* original_back = ctx.back_buffer;
    
    // Swap buffers
    display_driver_swap_buffers(&ctx);
    
    // Pointers should be swapped
    TEST_ASSERT_EQUAL(original_back, ctx.framebuffer);
    TEST_ASSERT_EQUAL(original_front, ctx.back_buffer);
    
    display_driver_deinit(&ctx);
}

// Test Error Handling
void test_display_driver_operations_with_null_context(void) {
    // All operations should handle null context gracefully
    display_driver_clear_screen(NULL, COLOR_RED);
    display_driver_draw_rectangle(NULL, 0, 0, 10, 10, COLOR_RED);
    display_driver_draw_text(NULL, 0, 0, "test", COLOR_RED);
    display_driver_swap_buffers(NULL);
    display_driver_flush(NULL);
    display_driver_set_pixel(NULL, 0, 0, COLOR_RED);
    
    // Get operations should return safe values
    TEST_ASSERT_EQUAL(0, display_driver_get_pixel(NULL, 0, 0));
    TEST_ASSERT_FALSE(display_driver_is_initialized(NULL));
}

void test_display_driver_operations_with_uninitialized_context(void) {
    display_context_t ctx = {0}; // Uninitialized context
    
    // Operations should handle uninitialized context gracefully
    display_driver_clear_screen(&ctx, COLOR_RED);
    display_driver_draw_rectangle(&ctx, 0, 0, 10, 10, COLOR_RED);
    display_driver_draw_text(&ctx, 0, 0, "test", COLOR_RED);
    display_driver_swap_buffers(&ctx);
    display_driver_flush(&ctx);
    display_driver_set_pixel(&ctx, 0, 0, COLOR_RED);
    
    TEST_ASSERT_EQUAL(0, display_driver_get_pixel(&ctx, 0, 0));
    TEST_ASSERT_FALSE(display_driver_is_initialized(&ctx));
}

void app_main(void) {
    UNITY_BEGIN();
    
    // Display Initialization Tests
    RUN_TEST(test_display_driver_init);
    RUN_TEST(test_display_driver_init_null_context);
    RUN_TEST(test_display_driver_deinit);
    RUN_TEST(test_display_driver_is_initialized);
    
    // Screen Clearing Tests
    RUN_TEST(test_display_driver_clear_screen);
    RUN_TEST(test_display_driver_clear_screen_different_colors);
    
    // Rectangle Drawing Tests
    RUN_TEST(test_display_driver_draw_rectangle);
    RUN_TEST(test_display_driver_draw_rectangle_clipping);
    RUN_TEST(test_display_driver_draw_rectangle_out_of_bounds);
    
    // Sprite Drawing Tests
    RUN_TEST(test_display_driver_draw_sprite);
    RUN_TEST(test_display_driver_draw_sprite_null);
    
    // Text Drawing Tests
    RUN_TEST(test_display_driver_draw_text);
    RUN_TEST(test_display_driver_draw_text_with_newline);
    
    // Pixel Operations Tests
    RUN_TEST(test_display_driver_set_get_pixel);
    RUN_TEST(test_display_driver_pixel_bounds_checking);
    
    // Buffer Management Tests
    RUN_TEST(test_display_driver_swap_buffers);
    
    // Error Handling Tests
    RUN_TEST(test_display_driver_operations_with_null_context);
    RUN_TEST(test_display_driver_operations_with_uninitialized_context);
    
    UNITY_END();
}
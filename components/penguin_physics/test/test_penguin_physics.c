#include "unity.h"
#include "penguin_physics.h"

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Test Penguin Movement - Rising when button not pressed
void test_penguin_physics_init(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    TEST_ASSERT_FLOAT_WITHIN(0.1, SCREEN_WIDTH / 6.0f, penguin.x);
    TEST_ASSERT_FLOAT_WITHIN(0.1, SCREEN_HEIGHT / 2.0f, penguin.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0f, penguin.velocity_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.5f, penguin.acceleration_y);
    TEST_ASSERT_FALSE(penguin.button_pressed);
}

void test_penguin_physics_rises_when_button_not_pressed(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    float initial_y = penguin.y;
    
    // Update several times without button press
    for (int i = 0; i < 10; i++) {
        penguin_physics_update(&penguin, false);
    }
    
    // Penguin should have moved up (smaller y value)
    TEST_ASSERT_LESS_THAN(initial_y, penguin.y);
}

// Test Dive Mechanics - Down movement on button press
void test_penguin_physics_dives_when_button_pressed(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    float initial_y = penguin.y;
    
    // Update several times with button press
    for (int i = 0; i < 10; i++) {
        penguin_physics_update(&penguin, true);
    }
    
    // Penguin should have moved down (larger y value)
    TEST_ASSERT_GREATER_THAN(initial_y, penguin.y);
}

void test_penguin_physics_button_press_duration_tracking(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    TEST_ASSERT_EQUAL(0, penguin.button_press_duration);
    
    // Press button for 5 frames
    for (int i = 0; i < 5; i++) {
        penguin_physics_update(&penguin, true);
        TEST_ASSERT_EQUAL(i + 1, penguin.button_press_duration);
    }
    
    // Release button
    penguin_physics_update(&penguin, false);
    TEST_ASSERT_EQUAL(0, penguin.button_press_duration);
}

void test_penguin_physics_immediate_dive_response(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    float initial_velocity = penguin.velocity_y;
    
    // First button press should give immediate velocity boost
    penguin_physics_update(&penguin, true);
    
    TEST_ASSERT_GREATER_THAN(initial_velocity, penguin.velocity_y);
}

// Test Smooth Physics - Velocity and acceleration calculations
void test_penguin_physics_velocity_clamping(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Force extreme velocity
    penguin.velocity_y = 20.0f; // Above max
    penguin_physics_update(&penguin, true);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, 10.0f, penguin.velocity_y); // Should be clamped to MAX_VELOCITY
    
    // Test negative extreme
    penguin.velocity_y = -20.0f; // Below min
    penguin_physics_update(&penguin, false);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, -10.0f, penguin.velocity_y); // Should be clamped to -MAX_VELOCITY
}

void test_penguin_physics_smooth_acceleration(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Test that acceleration affects velocity smoothly
    float initial_velocity = penguin.velocity_y;
    penguin_physics_update(&penguin, false);
    
    float velocity_change = penguin.velocity_y - initial_velocity;
    TEST_ASSERT_FLOAT_WITHIN(0.001, penguin.acceleration_y, velocity_change);
}

// Test Screen Boundaries - Penguin stays within screen bounds
void test_penguin_physics_screen_boundary_top(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Force penguin above screen
    penguin.y = -10.0f;
    penguin.velocity_y = -5.0f;
    
    penguin_physics_constrain_to_screen(&penguin);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0f, penguin.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0f, penguin.velocity_y);
}

void test_penguin_physics_screen_boundary_bottom(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Force penguin below screen
    penguin.y = SCREEN_HEIGHT;
    penguin.velocity_y = 5.0f;
    
    penguin_physics_constrain_to_screen(&penguin);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, SCREEN_HEIGHT - PENGUIN_HEIGHT, penguin.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0f, penguin.velocity_y);
}

void test_penguin_physics_screen_boundary_left(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    penguin.x = -10.0f;
    penguin_physics_constrain_to_screen(&penguin);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0f, penguin.x);
}

void test_penguin_physics_screen_boundary_right(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    penguin.x = SCREEN_WIDTH;
    penguin_physics_constrain_to_screen(&penguin);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, SCREEN_WIDTH - PENGUIN_WIDTH, penguin.x);
}

void test_penguin_physics_within_screen_bounds_check(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Normal position should be within bounds
    TEST_ASSERT_TRUE(penguin_physics_is_within_screen_bounds(&penguin));
    
    // Out of bounds positions
    penguin.y = -1.0f;
    TEST_ASSERT_FALSE(penguin_physics_is_within_screen_bounds(&penguin));
    
    penguin.y = SCREEN_HEIGHT;
    TEST_ASSERT_FALSE(penguin_physics_is_within_screen_bounds(&penguin));
}

// Test Screen Edge Detection for Game Over
void test_penguin_physics_screen_edge_detection_normal_position(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Normal position should not be at edge
    TEST_ASSERT_FALSE(penguin_physics_is_at_screen_edge(&penguin));
}

void test_penguin_physics_screen_edge_detection_left_edge(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Position penguin beyond left edge
    penguin.x = -1.0f;
    TEST_ASSERT_TRUE(penguin_physics_is_at_screen_edge(&penguin));
    
    // Position penguin exactly at left edge (should not be collision)
    penguin.x = 0.0f;
    TEST_ASSERT_FALSE(penguin_physics_is_at_screen_edge(&penguin));
}

void test_penguin_physics_screen_edge_detection_right_edge(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Position penguin beyond right edge
    penguin.x = SCREEN_WIDTH - PENGUIN_WIDTH + 1.0f;
    TEST_ASSERT_TRUE(penguin_physics_is_at_screen_edge(&penguin));
    
    // Position penguin exactly at right edge (should not be collision)
    penguin.x = SCREEN_WIDTH - PENGUIN_WIDTH;
    TEST_ASSERT_FALSE(penguin_physics_is_at_screen_edge(&penguin));
}

void test_penguin_physics_screen_edge_detection_top_edge(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Position penguin beyond top edge
    penguin.y = -1.0f;
    TEST_ASSERT_TRUE(penguin_physics_is_at_screen_edge(&penguin));
    
    // Position penguin exactly at top edge (should not be collision)
    penguin.y = 0.0f;
    TEST_ASSERT_FALSE(penguin_physics_is_at_screen_edge(&penguin));
}

void test_penguin_physics_screen_edge_detection_bottom_edge(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Position penguin beyond bottom edge
    penguin.y = SCREEN_HEIGHT - PENGUIN_HEIGHT + 1.0f;
    TEST_ASSERT_TRUE(penguin_physics_is_at_screen_edge(&penguin));
    
    // Position penguin exactly at bottom edge (should not be collision)
    penguin.y = SCREEN_HEIGHT - PENGUIN_HEIGHT;
    TEST_ASSERT_FALSE(penguin_physics_is_at_screen_edge(&penguin));
}

void test_penguin_physics_screen_edge_detection_corners(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Test corner collisions
    penguin.x = -1.0f;
    penguin.y = -1.0f;
    TEST_ASSERT_TRUE(penguin_physics_is_at_screen_edge(&penguin));
    
    penguin.x = SCREEN_WIDTH;
    penguin.y = SCREEN_HEIGHT;
    TEST_ASSERT_TRUE(penguin_physics_is_at_screen_edge(&penguin));
}

void test_penguin_physics_get_screen_coordinates(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    penguin.x = 25.7f;
    penguin.y = 100.3f;
    
    TEST_ASSERT_EQUAL(25, penguin_physics_get_screen_x(&penguin));
    TEST_ASSERT_EQUAL(100, penguin_physics_get_screen_y(&penguin));
}

void test_penguin_physics_button_state_tracking(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    // Initial state
    TEST_ASSERT_FALSE(penguin.button_pressed);
    TEST_ASSERT_FALSE(penguin.was_button_pressed);
    
    // Press button
    penguin_physics_update(&penguin, true);
    TEST_ASSERT_TRUE(penguin.button_pressed);
    TEST_ASSERT_FALSE(penguin.was_button_pressed);
    
    // Keep button pressed
    penguin_physics_update(&penguin, true);
    TEST_ASSERT_TRUE(penguin.button_pressed);
    TEST_ASSERT_TRUE(penguin.was_button_pressed);
    
    // Release button
    penguin_physics_update(&penguin, false);
    TEST_ASSERT_FALSE(penguin.button_pressed);
    TEST_ASSERT_TRUE(penguin.was_button_pressed);
}

void app_main(void) {
    UNITY_BEGIN();
    
    // Penguin Movement Tests
    RUN_TEST(test_penguin_physics_init);
    RUN_TEST(test_penguin_physics_rises_when_button_not_pressed);
    
    // Dive Mechanics Tests
    RUN_TEST(test_penguin_physics_dives_when_button_pressed);
    RUN_TEST(test_penguin_physics_button_press_duration_tracking);
    RUN_TEST(test_penguin_physics_immediate_dive_response);
    
    // Smooth Physics Tests
    RUN_TEST(test_penguin_physics_velocity_clamping);
    RUN_TEST(test_penguin_physics_smooth_acceleration);
    
    // Screen Boundaries Tests
    RUN_TEST(test_penguin_physics_screen_boundary_top);
    RUN_TEST(test_penguin_physics_screen_boundary_bottom);
    RUN_TEST(test_penguin_physics_screen_boundary_left);
    RUN_TEST(test_penguin_physics_screen_boundary_right);
    RUN_TEST(test_penguin_physics_within_screen_bounds_check);
    RUN_TEST(test_penguin_physics_get_screen_coordinates);
    
    // Screen Edge Detection Tests (for Game Over)
    RUN_TEST(test_penguin_physics_screen_edge_detection_normal_position);
    RUN_TEST(test_penguin_physics_screen_edge_detection_left_edge);
    RUN_TEST(test_penguin_physics_screen_edge_detection_right_edge);
    RUN_TEST(test_penguin_physics_screen_edge_detection_top_edge);
    RUN_TEST(test_penguin_physics_screen_edge_detection_bottom_edge);
    RUN_TEST(test_penguin_physics_screen_edge_detection_corners);
    
    // Button State Tests
    RUN_TEST(test_penguin_physics_button_state_tracking);
    
    UNITY_END();
}
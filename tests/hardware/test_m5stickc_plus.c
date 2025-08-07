#include "unity.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)
#endif
#include "display_driver.h"

// Hardware-specific tests for M5StickC Plus
#ifndef BUTTON_GPIO
#define BUTTON_GPIO 37
#endif
#define TEST_DELAY_MS 100

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Test GPIO37 Button Configuration
void test_gpio37_button_configuration(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, // GPIO37 is input-only, no internal PU
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t result = gpio_config(&io_conf);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Button should read HIGH when not pressed (pull-up enabled)
    int level = gpio_get_level(BUTTON_GPIO);
    TEST_ASSERT_EQUAL(1, level); // Assuming button not pressed during test
}

void test_gpio37_button_reading(void) {
    // Configure GPIO37 as input with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, // GPIO37 is input-only, no internal PU
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Read button state multiple times to ensure stability
    int readings[10];
    for (int i = 0; i < 10; i++) {
        readings[i] = gpio_get_level(BUTTON_GPIO);
    vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // All readings should be consistent (assuming button state doesn't change during test)
    for (int i = 1; i < 10; i++) {
        TEST_ASSERT_EQUAL(readings[0], readings[i]);
    }
}

// Test ST7789v2 Display SPI Communication
void test_st7789v2_spi_pins(void) {
    // Test that SPI pins are available and can be configured
    // This is a basic test to ensure pins are not conflicting
    
    gpio_config_t mosi_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_15),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config_t clk_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_13),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config_t cs_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_5),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config_t dc_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_23),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config_t rst_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_18),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    TEST_ASSERT_EQUAL(ESP_OK, gpio_config(&mosi_conf));
    TEST_ASSERT_EQUAL(ESP_OK, gpio_config(&clk_conf));
    TEST_ASSERT_EQUAL(ESP_OK, gpio_config(&cs_conf));
    TEST_ASSERT_EQUAL(ESP_OK, gpio_config(&dc_conf));
    TEST_ASSERT_EQUAL(ESP_OK, gpio_config(&rst_conf));
    
    // Test basic GPIO operations
    gpio_set_level(GPIO_NUM_5, 1);   // CS high
    gpio_set_level(GPIO_NUM_18, 1);  // RST high
    gpio_set_level(GPIO_NUM_23, 0);  // DC low
    
    TEST_ASSERT_EQUAL(1, gpio_get_level(GPIO_NUM_5));
    TEST_ASSERT_EQUAL(1, gpio_get_level(GPIO_NUM_18));
    TEST_ASSERT_EQUAL(0, gpio_get_level(GPIO_NUM_23));
}

void test_esp32_memory_allocation(void) {
    // Test memory allocation for game buffers
    size_t heap_before = esp_get_free_heap_size();
    
    // Allocate memory similar to display buffer
    size_t buffer_size = 135 * 240 * sizeof(uint16_t);
    void* test_buffer = malloc(buffer_size);
    
    TEST_ASSERT_NOT_NULL(test_buffer);
    
    size_t heap_after_alloc = esp_get_free_heap_size();
    TEST_ASSERT_GREATER_OR_EQUAL(buffer_size, heap_before - heap_after_alloc);
    
    }

    // Hardware display test: initialize, clear to red, flush
    void test_display_driver_hardware(void) {
        display_context_t ctx;
        bool ok = display_driver_init(&ctx);
        TEST_ASSERT_TRUE_MESSAGE(ok, "Display driver init failed");

        display_driver_clear_screen(&ctx, COLOR_RED);
        display_driver_swap_buffers(&ctx);
        display_driver_flush(&ctx);

        // Optionally, wait to allow visual confirmation
        vTaskDelay(pdMS_TO_TICKS(1000));

        display_driver_deinit(&ctx);
    // Free memory
    free(test_buffer);
    
    size_t heap_after_free = esp_get_free_heap_size();
    TEST_ASSERT_GREATER_OR_EQUAL(heap_before - 100, heap_after_free); // Allow some overhead
}

// Simple test task function
static void test_task_function(void* pvParameters) {
        // Display Hardware Test
        RUN_TEST(test_display_driver_hardware);
    (void)pvParameters;
    
    // Simple task that just delays and exits
    vTaskDelay(pdMS_TO_TICKS(10));
    vTaskDelete(NULL);
}

// Test FreeRTOS Task Creation
void test_freertos_task_creation(void) {
    TaskHandle_t test_task_handle = NULL;
    
    // Create a simple test task
    BaseType_t result = xTaskCreate(
        test_task_function,
        "test_task",
        2048,
        NULL,
        1,
        &test_task_handle
    );
    
    TEST_ASSERT_EQUAL(pdPASS, result);
    TEST_ASSERT_NOT_NULL(test_task_handle);
    
    // Let task run briefly
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Delete task
    vTaskDelete(test_task_handle);
}

// Test Timer Functions for Game Timing
void test_game_timing_functions(void) {
    TickType_t start_tick = xTaskGetTickCount();
    
    vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    
    TickType_t end_tick = xTaskGetTickCount();
    TickType_t elapsed_ticks = end_tick - start_tick;
    
    // Convert back to milliseconds
    uint32_t elapsed_ms = elapsed_ticks * portTICK_PERIOD_MS;
    
    // Should be approximately TEST_DELAY_MS (allow some tolerance)
    TEST_ASSERT_GREATER_OR_EQUAL(TEST_DELAY_MS - 10, elapsed_ms);
    TEST_ASSERT_LESS_OR_EQUAL(TEST_DELAY_MS + 10, elapsed_ms);
}

// Test Button Response Time
void test_button_response_time(void) {
    // Configure GPIO37 for button input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, // GPIO37 is input-only, no internal PU
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Measure response time by reading GPIO multiple times rapidly
    const int num_reads = 1000;
    TickType_t start_time = xTaskGetTickCount();
    
    for (int i = 0; i < num_reads; i++) {
        gpio_get_level(BUTTON_GPIO);
    }
    
    TickType_t end_time = xTaskGetTickCount();
    uint32_t total_time_ms = (end_time - start_time) * portTICK_PERIOD_MS;
    
    // Each read should take much less than 1ms
    float avg_read_time = (float)total_time_ms / num_reads;
    TEST_ASSERT_LESS_THAN(1.0f, avg_read_time);
}

void app_main(void) {
    UNITY_BEGIN();
    
    // GPIO Tests
    RUN_TEST(test_gpio37_button_configuration);
    RUN_TEST(test_gpio37_button_reading);
    
    // Display SPI Tests
    RUN_TEST(test_st7789v2_spi_pins);
    
    // System Tests
    RUN_TEST(test_esp32_memory_allocation);
    
    // FreeRTOS Tests
    RUN_TEST(test_freertos_task_creation);
    RUN_TEST(test_game_timing_functions);
    
    // Performance Tests
    RUN_TEST(test_button_response_time);
    
    UNITY_END();
}
#include "display_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include "esp_rom_gpio.h"

#include "lvgl.h"

static const char *TAG = "display_driver";

// LVGL display and buffer objects
static lv_disp_t *lvgl_display = NULL;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// Canvas for direct drawing
static lv_obj_t *canvas = NULL;
static lv_color_t *canvas_buf = NULL;

// SPI handle for communication
static spi_device_handle_t spi_handle = NULL;

// Function prototypes for LVGL callbacks and ST7789 communication
static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
static void display_spi_write_cmd(uint8_t cmd);
static void display_spi_write_data(const uint8_t *data, size_t len);
static uint8_t display_spi_read_data(void);
static uint8_t display_read_register(uint8_t reg);
static void display_set_window(int x, int y, int width, int height);
static void display_init_st7789(void);

bool display_driver_init(display_context_t *ctx) {
    if (!ctx) {
        ESP_LOGE(TAG, "Invalid display context");
        return false;
    }

    ESP_LOGI(TAG, "Initializing LVGL ST7789v2 display driver");

    // Initialize LVGL
    lv_init();

    // Power stabilization delay before SPI init
    ESP_LOGI(TAG, "Waiting for display power stabilization...");
    vTaskDelay(pdMS_TO_TICKS(50));

    // Initialize SPI bus configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI_PIN,
        .miso_io_num = TFT_MISO_PIN, // Set to actual MISO pin number
        .sclk_io_num = TFT_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * 64 * 2, // Match LVGL buffer size
    };

    // Initialize SPI device configuration for ST7789 compatibility
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // Reduced to 10 MHz for reliability
        .mode = 0,  // SPI Mode 0 (CPOL=0, CPHA=0) - ST7789 standard
        .spics_io_num = TFT_CS_PIN,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY,  // Half-duplex, no dummy phase
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 16, // ST7789 requires 16 dummy bits for reads
        .duty_cycle_pos = 128,  // 50% duty cycle
    };
    
    ESP_LOGI(TAG, "SPI Config: Clock=%lu Hz, Mode=%d, CS=GPIO%d, Flags=0x%08lX", 
             devcfg.clock_speed_hz, devcfg.mode, devcfg.spics_io_num, devcfg.flags);

    // Initialize SPI bus (use VSPI_HOST for compatibility)
    esp_err_t ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }

    // Add device to SPI bus (use VSPI_HOST)
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(VSPI_HOST);
        return false;
    }

    // Configure DC and RST pins
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << TFT_DC_PIN) | (1ULL << TFT_RST_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;  // Enable pull-up for more reliable signaling
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Starting display reset sequence...");
    
    // Extended reset sequence for ST7789 reliability
    // Start with reset high
    gpio_set_level(TFT_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Assert reset (low) for longer period
    gpio_set_level(TFT_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));  // Increased from 10ms to 50ms
    
    // Deassert reset and wait longer for startup
    gpio_set_level(TFT_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(200)); // Increased from 120ms to 200ms
    
    ESP_LOGI(TAG, "Display reset sequence complete");

    // Initialize ST7789 display
    display_init_st7789();
    
    // Verify ST7789 communication by reading some registers
    ESP_LOGI(TAG, "Verifying ST7789 communication...");
    ESP_LOGI(TAG, "Testing basic register reads...");
    
    // Add delay before first read
    vTaskDelay(pdMS_TO_TICKS(10));
    
    uint8_t reg04 = display_read_register(0x04); // Read Display ID
    uint8_t reg09 = display_read_register(0x09); // Read Display Status  
    uint8_t reg0A = display_read_register(0x0A); // Read Display Power Mode
    uint8_t reg0C = display_read_register(0x0C); // Read Display Pixel Format
    
    ESP_LOGI(TAG, "Register readback results: 0x04=0x%02X, 0x09=0x%02X, 0x0A=0x%02X, 0x0C=0x%02X", 
             reg04, reg09, reg0A, reg0C);
    
    if (reg04 == 0x00 && reg09 == 0x00 && reg0A == 0x00 && reg0C == 0x00) {
        ESP_LOGW(TAG, "All registers returned 0x00 - ST7789 may not be responding!");
        ESP_LOGW(TAG, "This could indicate SPI communication issues or hardware problems");
    } else {
        ESP_LOGI(TAG, "ST7789 appears to be responding to SPI commands");
    }

    // Allocate LVGL draw buffers
    size_t buf_size = DISPLAY_WIDTH * DISPLAY_HEIGHT / 10; // 1/10th screen size for memory efficiency
    buf1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);

    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL display buffers");
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        spi_bus_remove_device(spi_handle);
        spi_bus_free(VSPI_HOST);
        return false;
    }

    // Initialize LVGL draw buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size);

    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    
    // Register display driver
    lvgl_display = lv_disp_drv_register(&disp_drv);
    if (!lvgl_display) {
        ESP_LOGE(TAG, "Failed to register LVGL display driver");
        free(buf1);
        free(buf2);
        spi_bus_remove_device(spi_handle);
        spi_bus_free(VSPI_HOST);
        return false;
    }

    // Skip canvas creation for now - we'll draw directly on screen objects
    canvas_buf = NULL;
    canvas = NULL;

    // Store the LVGL display in context for compatibility
    ctx->lvgl_display = (struct _lv_display_t *)lvgl_display;
    ctx->initialized = true;

    ESP_LOGI(TAG, "LVGL display driver initialized successfully");
    return true;
}

void display_driver_deinit(display_context_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }

    // Free canvas buffer
    if (canvas_buf) {
        free(canvas_buf);
        canvas_buf = NULL;
    }
    
    // Delete canvas object
    if (canvas) {
        lv_obj_del(canvas);
        canvas = NULL;
    }

    // Free LVGL buffers
    if (buf1) {
        free(buf1);
        buf1 = NULL;
    }
    if (buf2) {
        free(buf2);
        buf2 = NULL;
    }

    // LVGL display is automatically cleaned up when deinitializing
    lvgl_display = NULL;

    // Remove SPI device and free bus
    if (spi_handle) {
        spi_bus_remove_device(spi_handle);
        spi_bus_free(VSPI_HOST);
        spi_handle = NULL;
    }

    ctx->initialized = false;
    ESP_LOGI(TAG, "Display driver deinitialized");
}

void display_driver_clear_screen(display_context_t *ctx, uint16_t color) {
    if (!ctx || !ctx->initialized) {
        ESP_LOGE(TAG, "Display context not initialized");
        return;
    }

    ESP_LOGI(TAG, "Setting background color to 0x%04X", color);

    // Test multiple color conversion methods
    
    // Method 1: lv_color_hex (current problematic method)
    lv_color_t lv_color_hex_result = lv_color_hex(color);
    ESP_LOGI(TAG, "Method 1 - lv_color_hex(0x%04X): R=%d G=%d B=%d", 
             color, lv_color_hex_result.ch.red, lv_color_hex_result.ch.green, lv_color_hex_result.ch.blue);
    
    // Method 2: Manual RGB565 bit extraction
    uint8_t r = (color >> 11) & 0x1F;  // 5 bits
    uint8_t g = (color >> 5) & 0x3F;   // 6 bits  
    uint8_t b = color & 0x1F;          // 5 bits
    // Scale to 8-bit values
    uint8_t r8 = (r * 255) / 31;
    uint8_t g8 = (g * 255) / 63; 
    uint8_t b8 = (b * 255) / 31;
    lv_color_t lv_color_manual = lv_color_make(r8, g8, b8);
    ESP_LOGI(TAG, "Method 2 - Manual RGB565 extract: RGB565(%d,%d,%d) -> RGB888(%d,%d,%d)", 
             r, g, b, r8, g8, b8);
    ESP_LOGI(TAG, "Method 2 - lv_color_make result: R=%d G=%d B=%d", 
             lv_color_manual.ch.red, lv_color_manual.ch.green, lv_color_manual.ch.blue);
    
    // Method 3: Convert RGB565 to RGB888 hex first  
    uint32_t rgb888 = ((r8 << 16) | (g8 << 8) | b8);
    lv_color_t lv_color_rgb888 = lv_color_hex(rgb888);
    ESP_LOGI(TAG, "Method 3 - RGB565->RGB888 hex (0x%06lX): R=%d G=%d B=%d",
             rgb888, lv_color_rgb888.ch.red, lv_color_rgb888.ch.green, lv_color_rgb888.ch.blue);
    
    // Use Method 2 (manual extraction) for now as it should be most accurate
    lv_color_t lv_color = lv_color_manual;
    
    // Set background color  
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
    
    // Force LVGL to invalidate and refresh
    lv_obj_invalidate(lv_scr_act());
    
    ESP_LOGI(TAG, "Background color set and screen invalidated");
}

void display_driver_draw_rectangle(display_context_t *ctx, int x, int y, int width, int height, uint16_t color) {
    if (!ctx || !ctx->initialized) {
        return;
    }

    // Convert RGB565 to LVGL color - use reliable lv_color_hex
    lv_color_t lv_color = lv_color_hex(color);
    
    // Create a rectangle object
    lv_obj_t *rect = lv_obj_create(lv_scr_act());
    lv_obj_set_pos(rect, x, y);
    lv_obj_set_size(rect, width, height);
    
    // Set rectangle style
    lv_obj_set_style_bg_color(rect, lv_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(rect, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(rect, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(rect, 0, LV_PART_MAIN);
}

void display_driver_draw_text(display_context_t *ctx, int x, int y, const char *text, uint16_t color) {
    if (!ctx || !ctx->initialized || !text) {
        return;
    }

    // Convert RGB565 to LVGL color - use reliable lv_color_hex
    lv_color_t lv_color = lv_color_hex(color);
    
    // Create a label for text
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_obj_set_pos(label, x, y);
    lv_label_set_text(label, text);
    
    // Set text color and remove padding
    lv_obj_set_style_text_color(label, lv_color, LV_PART_MAIN);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN);
}

void display_driver_swap_buffers(display_context_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }
    // With LVGL, buffer swapping is handled automatically
    // This function is kept for compatibility but does nothing
}

void display_driver_flush(display_context_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return;
    }

    // LVGL handles refresh automatically through lv_timer_handler()
    // Manual refresh can interfere with LVGL's internal timing
    // The timer handler in main loop will trigger refreshes as needed
}

// LVGL flush callback for ST7789 communication
static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    if (!spi_handle || !area || !color_p) {
        ESP_LOGE(TAG, "Flush callback: Invalid parameters (spi_handle=%p, area=%p, color_p=%p)", 
                 spi_handle, area, color_p);
        lv_disp_flush_ready(disp_drv);
        return;
    }

    // Debug: Log flush operations to verify data is being sent
    size_t pixel_count = lv_area_get_size(area);
    ESP_LOGI(TAG, "FLUSH CALLED: area (%d,%d) to (%d,%d), %d pixels, first_pixel=0x%04X", 
             area->x1, area->y1, area->x2, area->y2, pixel_count, 
             pixel_count > 0 ? *(uint16_t*)color_p : 0);
    
    // Hex dump first 16 bytes of pixel data
    if (pixel_count > 0) {
        uint8_t *data = (uint8_t *)color_p;
        ESP_LOGI(TAG, "SPI Data Hex: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                 data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
        
        // Also show as 16-bit RGB565 values  
        uint16_t *pixels = (uint16_t *)color_p;
        ESP_LOGI(TAG, "RGB565 Values: 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X",
                 pixels[0], pixels[1], pixels[2], pixels[3], pixels[4], pixels[5], pixels[6], pixels[7]);
    }

    // Set window for the area to be flushed
    display_set_window(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area));

    // Memory write (0x2C)
    display_spi_write_cmd(0x2C);

    // Send pixel data
    size_t buf_size = pixel_count * sizeof(lv_color_t);
    display_spi_write_data((uint8_t *)color_p, buf_size);
    
    ESP_LOGI(TAG, "Sent %d bytes to SPI", buf_size);

    // Tell LVGL that flushing is done
    lv_disp_flush_ready(disp_drv);
}

// Helper functions for SPI communication with proper timing
static void display_spi_write_cmd(uint8_t cmd) {
    if (!spi_handle) return;
    
    // Set DC low for command mode
    gpio_set_level(TFT_DC_PIN, 0);
    // Small delay to ensure DC line is stable
    esp_rom_delay_us(1);
    
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    
    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI command transmission failed: %s", esp_err_to_name(ret));
    }
    
    // Small delay after command
    esp_rom_delay_us(1);
}

static void display_spi_write_data(const uint8_t *data, size_t len) {
    if (!spi_handle || !data || len == 0) return;
    
    // Set DC high for data mode
    gpio_set_level(TFT_DC_PIN, 1);
    // Small delay to ensure DC line is stable
    esp_rom_delay_us(1);
    
    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    
    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI data transmission failed: %s", esp_err_to_name(ret));
    }
    
    // Small delay after data
    esp_rom_delay_us(1);
}

static uint8_t display_spi_read_data(void) {
    if (!spi_handle) return 0;

    // Set DC high for data mode
    gpio_set_level(TFT_DC_PIN, 1);
    esp_rom_delay_us(1);

    uint8_t rx_data = 0;
    spi_transaction_t trans = {
        .flags = 0,
        .length = 8,
        .rxlength = 8,
        .rx_buffer = &rx_data,
        .tx_buffer = NULL
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI read transmission failed: %s", esp_err_to_name(ret));
        return 0;
    }

    esp_rom_delay_us(1);
    return rx_data;
}

static uint8_t display_read_register(uint8_t reg) {
    if (!spi_handle) return 0;

    ESP_LOGI(TAG, "Reading ST7789 register 0x%02X", reg);

    // Send read command (DC low)
    gpio_set_level(TFT_DC_PIN, 0);
    esp_rom_delay_us(2);
    display_spi_write_cmd(reg);
    esp_rom_delay_us(2);

    // Set DC high for data phase
    gpio_set_level(TFT_DC_PIN, 1);
    esp_rom_delay_us(2);

    // Read response with dummy bits
    uint8_t result = display_spi_read_data();

    ESP_LOGI(TAG, "Register 0x%02X = 0x%02X", reg, result);
    vTaskDelay(pdMS_TO_TICKS(2)); // Datasheet recommends short delay after read
    return result;
}

static void display_set_window(int x, int y, int width, int height) {
    if (!spi_handle) return;
    
    // Apply M5StickC Plus display offsets
    uint16_t x1 = x + DISPLAY_OFFSET_X;
    uint16_t x2 = x + width - 1 + DISPLAY_OFFSET_X;
    uint16_t y1 = y + DISPLAY_OFFSET_Y;
    uint16_t y2 = y + height - 1 + DISPLAY_OFFSET_Y;

    ESP_LOGD(TAG, "Setting window: (%d,%d)+offset -> (%d,%d) to (%d,%d)", 
             x, y, x1, y1, x2, y2);

    // Column address set (0x2A)
    display_spi_write_cmd(0x2A);
    uint8_t caset_data[4] = {x1 >> 8, x1 & 0xFF, x2 >> 8, x2 & 0xFF};
    display_spi_write_data(caset_data, 4);

    // Row address set (0x2B)
    display_spi_write_cmd(0x2B);
    uint8_t raset_data[4] = {y1 >> 8, y1 & 0xFF, y2 >> 8, y2 & 0xFF};
    display_spi_write_data(raset_data, 4);
}

// ST7789 display initialization sequence with proper timing
static void display_init_st7789(void) {
    ESP_LOGI(TAG, "Starting ST7789 initialization sequence...");

    // Software reset
    ESP_LOGI(TAG, "Sending software reset command...");
    display_spi_write_cmd(0x01); // SWRESET
    vTaskDelay(pdMS_TO_TICKS(130)); // 130ms per working example

    // Sleep out
    ESP_LOGI(TAG, "Sending sleep out command...");
    display_spi_write_cmd(0x11); // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(130)); // 130ms per working example

    // Porch control
    display_spi_write_cmd(0xB2); // PORCTRL
    uint8_t porctrl_data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    display_spi_write_data(porctrl_data, sizeof(porctrl_data));

    // Gate control
    display_spi_write_cmd(0xB7); // GCTRL
    uint8_t gctrl_data = 0x35;
    display_spi_write_data(&gctrl_data, 1);

    // VCOM setting
    display_spi_write_cmd(0xBB); // VCOMS
    uint8_t vcoms_data = 0x28;
    display_spi_write_data(&vcoms_data, 1);

    // LCM control
    display_spi_write_cmd(0xC0); // LCMCTRL
    uint8_t lcmctrl_data = 0x0C;
    display_spi_write_data(&lcmctrl_data, 1);

    // VDV and VRH command enable
    display_spi_write_cmd(0xC2); // VDVVRHEN
    uint8_t vdvvrhen_data[] = {0x01, 0xFF};
    display_spi_write_data(vdvvrhen_data, 2);

    // VRHS set
    display_spi_write_cmd(0xC3); // VRHS
    uint8_t vrhs_data = 0x10;
    display_spi_write_data(&vrhs_data, 1);

    // VDV setting
    display_spi_write_cmd(0xC4); // VDVSET
    uint8_t vdvset_data = 0x20;
    display_spi_write_data(&vdvset_data, 1);

    // FR Control 2
    display_spi_write_cmd(0xC6); // FRCTR2
    uint8_t frctr2_data = 0x0F;
    display_spi_write_data(&frctr2_data, 1);

    // Power control 1
    display_spi_write_cmd(0xD0); // PWCTRL1
    uint8_t pwctrl1_data[] = {0xA4, 0xA1};
    display_spi_write_data(pwctrl1_data, 2);

    // RAM control
    display_spi_write_cmd(0xB0); // RAMCTRL
    uint8_t ramctrl_data[] = {0x00, 0xC0};
    display_spi_write_data(ramctrl_data, 2);

    // Positive voltage gamma control
    display_spi_write_cmd(0xE0); // PVGAMCTRL
    uint8_t pvgamctrl_data[] = {0xD0,0x00,0x02,0x07,0x0A,0x28,0x32,0x44,0x42,0x06,0x0E,0x12,0x14,0x17};
    display_spi_write_data(pvgamctrl_data, 14);

    // Negative voltage gamma control
    display_spi_write_cmd(0xE1); // NVGAMCTRL
    uint8_t nvgamctrl_data[] = {0xD0,0x00,0x02,0x07,0x0A,0x28,0x31,0x54,0x47,0x0E,0x1C,0x17,0x1B,0x1E};
    display_spi_write_data(nvgamctrl_data, 14);

    // Sleep out (again, per working example)
    display_spi_write_cmd(0x11); // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(130));

    // Display on
    display_spi_write_cmd(0x29); // DISPON
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "ST7789 display initialization complete");
}

// LVGL timer handler - should be called regularly in main loop
void display_driver_task_handler(void) {
    lv_timer_handler();
}

uint16_t display_driver_get_pixel(display_context_t *ctx, int x, int y) {
    if (!ctx || !ctx->initialized) {
        return 0;
    }
    
    // Check bounds
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return 0;
    }
    
    // For hardware LVGL implementation, we can't easily read back pixels
    // This function is mainly for simulator testing
    // Return a default value to satisfy compilation
    return 0;
}
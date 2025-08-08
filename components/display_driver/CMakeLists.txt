if (DEFINED IDF_TARGET)
    # Building under ESP-IDF for hardware: use M5Unified backend
    set(srcs "src/display_driver_m5.cpp")
    set(public_reqs m5unified)
    # Keep esp drivers available via transitive deps; M5Unified pulls required ones.
else()
    # Non-ESP builds (e.g., simulator toolchain) use legacy C driver (LVGL/SPI path)
    set(srcs "src/display_driver.c")
    set(public_reqs lvgl esp_driver_spi esp_driver_gpio)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    REQUIRES ${public_reqs}
    PRIV_REQUIRES freertos
)
idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES 
                           spi_flash
                           esp_driver_gpio
                           esp_driver_spi
                           game_engine
                           penguin_physics
                           ice_pillars
                           display_driver
                       INCLUDE_DIRS "")

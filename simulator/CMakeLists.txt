cmake_minimum_required(VERSION 3.14)

project(penguin_simulator LANGUAGES C)

# Find SDL2 using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Use CMAKE_PREFIX_PATH to help find libraries
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /opt/homebrew /usr/local)

# Set C standard
set(CMAKE_C_STANDARD 11)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/game_engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/penguin_physics/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/ice_pillars/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/display_driver/include
    ${SDL2_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    main.c
    ../components/game_engine/src/game_engine.c
    ../components/penguin_physics/src/penguin_physics.c
    ../components/ice_pillars/src/ice_pillars.c
    ../components/display_driver/src/display_driver.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries using full LDFLAGS
target_link_libraries(${PROJECT_NAME} ${SDL2_LDFLAGS})

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER})

# Test executable
add_executable(penguin_simulator_tests
    test_main.c
    ../components/game_engine/src/game_engine.c
    ../components/penguin_physics/src/penguin_physics.c
    ../components/ice_pillars/src/ice_pillars.c
    ../components/display_driver/src/display_driver.c
)

target_link_libraries(penguin_simulator_tests ${SDL2_LDFLAGS})
target_compile_options(penguin_simulator_tests PRIVATE ${SDL2_CFLAGS_OTHER})

# Enable testing
enable_testing()
add_test(NAME penguin_tests COMMAND penguin_simulator_tests)
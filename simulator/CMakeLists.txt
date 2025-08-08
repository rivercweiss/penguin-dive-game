cmake_minimum_required(VERSION 3.14)

project(penguin_simulator LANGUAGES C CXX)

# Find SDL2 using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Use CMAKE_PREFIX_PATH to help find libraries
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /opt/homebrew /usr/local)


# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Include directories for both executables
set(GAME_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/game_engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/penguin_physics/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/ice_pillars/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/display_driver/include
    ${SDL2_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    main.cpp
    ../components/game_engine/src/game_engine.c
    ../components/penguin_physics/src/penguin_physics.c
    ../components/ice_pillars/src/ice_pillars.c
    display_driver_sim.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${GAME_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LDFLAGS})
target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER})

# Test executable
add_executable(penguin_simulator_tests
    test_main.cpp
    ../components/game_engine/src/game_engine.c
    ../components/penguin_physics/src/penguin_physics.c
    ../components/ice_pillars/src/ice_pillars.c
    display_driver_sim.c
)
target_include_directories(penguin_simulator_tests PRIVATE ${GAME_INCLUDE_DIRS})
target_link_libraries(penguin_simulator_tests ${SDL2_LDFLAGS})
target_compile_options(penguin_simulator_tests PRIVATE ${SDL2_CFLAGS_OTHER})

# Enable testing
enable_testing()
add_test(NAME penguin_tests COMMAND penguin_simulator_tests)
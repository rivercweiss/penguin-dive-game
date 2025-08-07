# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Include our custom components directory
set(EXTRA_COMPONENT_DIRS 
    components/game_engine
    components/penguin_physics
    components/ice_pillars
    components/display_driver
)

# Remove minimal build to include Unity testing framework
# idf_build_set_property(MINIMAL_BUILD ON)

project(penguin-dive-game)

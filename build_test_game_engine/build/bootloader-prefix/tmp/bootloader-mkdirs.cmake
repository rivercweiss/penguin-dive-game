# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/riverweiss/esp/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/Users/riverweiss/esp/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader"
  "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader-prefix"
  "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader-prefix/tmp"
  "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader-prefix/src"
  "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/riverweiss/esp/penguin-dive-game/build_test_game_engine/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()

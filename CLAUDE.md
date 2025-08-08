# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## ESP-IDF Development Commands

### Environment Setup
```bash
. $HOME/esp/esp-idf/export.sh  # Required before building if environment not set up
```

### Build and Flash Commands
```bash
idf.py build                                           # Build the project
idf.py -b 1500000 -p /dev/cu.usbserial-* flash        # Flash with high baud rate
idf.py -p /dev/cu.usbserial-* monitor                  # Monitor serial output
idf.py -p /dev/cu.usbserial-* flash monitor            # Flash and monitor in one command
idf.py clean                                           # Clean build files
ls /dev/cu.*                                          # Find connected device port
```

### Project Structure
- `main/main.c` - Main application entry point
- `main/CMakeLists.txt` - Main component build configuration
- `CMakeLists.txt` - Project-level build configuration
- `sdkconfig` - ESP-IDF configuration file
- `build/` - Generated build artifacts (auto-created)

### Hardware Configuration
- Target: ESP32 M5StickC Plus
- Display: 1.14 inch 135x240 TFT LCD (ST7789v2)
- Input: Button A on GPIO37
- USB Port: Typically `/dev/cu.usbserial-*` format

## Project Overview
This project is a one button game called Diving Penguin. Diving Penguin is similar to Flappy Bird, but instead of a bird flying over pipes, this game will have a penguin diving under ice pillars. 

## Development Workflow
This will be a test driven development project. We will ONLY test to the requirements, NEVER testing implementation details of the code. We MUST be able to completely refactor the code with NONE of the tests breaking or changing.

We are using Claude to develop this project, so we must be able to emulate the code and visual output for Claude to inspect and run test on. We are planning to use LVGL + SDL for this purpose, with QEMU if needed for core logic.

## Hardware
The game will be implemented on an ESP32  m5stickc plus device. This device has documentation here: https://docs.m5stack.com/en/core/m5stickc_plus

The main components of the device we are interested in are the LCD screen: a 1.14 inch, 135 x 240 Colorful TFT LCD, ST7789v2: and button A, connected to GPIO37. This button is always referred to simply as the button.

### 135 x 240 Colorful TFT LCD, ST7789v2 Specifics
Color TFT Screen
Driver Chip: ST7789v2
Resolution: 135 x 240
Pinout:
TFT_MOSI, GPIO15
TFT_CLK, GPIO13
TFT_DC, GPIO23
TFT_RST, GPIO18
TFT_CS, GPIO5
A reference implentation is in the example_display folder.

## Key Gameplay Requirements

### Game Objective
- The goal of the game is to get as far as possible without the penguin contacting an ice pillar. When this happens, the score for how far the player made it is shown, and the game restarts.
- The game should also be lost if the player makes the penguin hit the edges of the screen.
- The penguin will swim through gaps in ice pillars, diving each time the button is pressed and rising otherwise. The pillars will get harder and harder to navigate though as the game progresses, and move faster and faster.

### Game Setup and Mechanics
- There is a penguin sprite
	- This penguin sprite stays on the left side of the screen
    - The penguin sprite moves up whenever the button is not pressed
    - On each button press, the penguin moves down by an amount. This amount should allow smooth gameplay. This amount could potentially depend on how long the button is held down.
    - The penguins movement is smooth and physics based
- There are ice pillar sprites
	- These ice pillars come from both the top and the bottom of the screen, and there is space between the pillars in the middle
    - There can be multiple sets of ice pillars on screen at one time.
    - The pillars will get harder and harder to navigate though as the game progresses, and move faster and faster.

## Testing Framework Plan

### Testing Architecture Overview
We implement a dual-environment testing framework supporting both **desktop simulation** (for rapid development) and **hardware testing** (for final validation), following strict TDD principles.

### Core Testing Stack
- **ESP-IDF Unity Framework**: For unit tests and component testing
- **LVGL + SDL Simulator**: For visual/gameplay testing on desktop  
- **Hardware-in-the-loop Tests**: For M5StickC Plus validation

### Project Structure
```
penguin-dive-game/
├── components/
│   ├── game_engine/          # Core game logic (testable)
│   │   ├── src/
│   │   ├── include/
│   │   └── test/             # Unity tests
│   ├── penguin_physics/      # Penguin movement & physics
│   │   ├── src/
│   │   ├── include/
│   │   └── test/
│   ├── ice_pillars/          # Pillar generation & collision
│   │   ├── src/
│   │   ├── include/
│   │   └── test/
│   └── display_driver/       # M5StickC Plus display abstraction
│       ├── src/
│       ├── include/
│       └── test/
├── simulator/                # LVGL + SDL desktop environment
│   ├── CMakeLists.txt
│   ├── main.c               # Desktop test runner
│   └── lv_conf.h            # LVGL configuration
├── tests/
│   ├── integration/         # Full game behavior tests
│   ├── hardware/           # Device-specific tests
│   └── requirements/       # Requirement-based test scenarios
└── main/                   # ESP32 application entry point
```

### Test Categories & Requirements Coverage

#### Core Game Logic Tests (component/game_engine/test/)
- **Game State Management**: Start, playing, game over, restart
- **Score Tracking**: Distance calculation and scoring 
- **Collision Detection**: Penguin-pillar collision logic and screen edge collision
- **Game Progression**: Increasing difficulty and speed

#### Physics & Movement Tests (component/penguin_physics/test/)
- **Penguin Movement**: Up movement when button not pressed
- **Dive Mechanics**: Down movement on button press
- **Smooth Physics**: Velocity and acceleration calculations
- **Screen Boundaries**: Penguin stays within screen bounds and screen edge detection

#### Ice Pillar Tests (component/ice_pillars/test/)
- **Pillar Generation**: Top/bottom pillars with gaps
- **Multiple Pillars**: Multiple sets on screen simultaneously  
- **Pillar Movement**: Left-to-right scrolling
- **Difficulty Progression**: Gap size reduction, speed increase

#### Integration Tests (tests/integration/)
- **Complete Gameplay Loop**: Full game from start to game over
- **Button Integration**: GPIO37 input handling
- **Visual Rendering**: Display output validation
- **Performance**: Frame rate and memory usage

### Testing Environments

#### Desktop Simulator Environment
- **Purpose**: Rapid TDD cycles, visual debugging, automated CI
- **Setup**: CMake project with SDL2 + LVGL
- **Benefits**: Fast iteration, screen capture for test validation
- **Test Execution**: `cmake build && ./penguin_simulator_tests`

#### Hardware Testing Environment
- **Purpose**: Final validation on M5StickC Plus hardware
- **Setup**: ESP-IDF Unity tests running on device
- **Hardware Features**: ST7789v2 display, GPIO37 button, ESP32 target
- **Test Execution**: `idf.py build && idf.py flash monitor`

### TDD Workflow Implementation

#### Test-First Development Process
1. **Write Failing Test**: Based on requirements, not implementation
2. **Run Test** (should fail): Verify test is correctly written
3. **Implement Minimum Code**: Make test pass
4. **Refactor**: Clean up code while keeping tests green
5. **Repeat**: For next requirement

#### Test Commands
```bash
# Desktop simulator tests (fast iteration)
cd simulator && cmake build && make test

# Component unit tests on hardware
idf.py build flash monitor

# Full integration test suite  
idf.py build && ./run_all_tests.sh
```

### Test Data & Validation

#### Visual Test Validation
- **Frame Capture**: SDL screenshot capability for visual regression
- **Sprite Positioning**: Verify penguin/pillar coordinates
- **Animation Smoothness**: Frame-by-frame movement validation

#### Behavioral Test Scenarios
- **Button Response Time**: GPIO37 input latency measurement
- **Collision Accuracy**: Pixel-perfect collision detection
- **Score Consistency**: Repeatable scoring under identical conditions

## Testing Framework Implementation Status

### ✅ Completed Components
The complete testing framework has been implemented and verified:

#### Component Architecture
- **game_engine**: Core game logic, state management, collision detection, scoring
- **penguin_physics**: Movement physics, diving mechanics, screen boundaries
- **ice_pillars**: Pillar generation, collision detection, difficulty progression  
- **display_driver**: ST7789v2 display abstraction with double buffering

#### Test Coverage
- **60+ Unit Tests**: Comprehensive coverage of all requirements
- **Integration Tests**: Complete gameplay loop validation
- **Hardware Tests**: M5StickC Plus GPIO37 and display testing
- **Requirements Tests**: Direct mapping to README specifications

#### Test Execution Tools
```bash
./run_all_tests.sh                    # Run complete test suite
./run_component_test.sh <component>    # Test individual component
./run_simulator.sh                     # Desktop game simulator (requires SDL2)
```

### Test Verification Results
All ESP-IDF component tests **compile successfully** and are ready for TDD development:
- ✅ `game_engine` component builds and links with Unity
- ✅ `penguin_physics` component builds and links with Unity  
- ✅ `ice_pillars` component builds and links with Unity
- ✅ `display_driver` component builds and links with Unity

### TDD-Ready Development
The framework supports the complete TDD workflow:
1. Tests are written based on requirements (not implementation)
2. Tests currently **compile but logic is not implemented** - perfect for TDD
3. Code can be completely refactored without breaking tests
4. Both desktop simulation and hardware validation are supported

### Next Development Steps
1. **Start TDD cycle**: Pick any failing test and implement minimum code to pass
2. **Run individual tests**: `./run_component_test.sh game_engine` 
3. **Flash and test on hardware**: `idf.py -b 1500000 -p /dev/cu.usbserial-* flash monitor`
4. **Use desktop simulator**: Run `./run_simulator.sh` for visual testing


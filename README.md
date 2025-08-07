# Project Overview
This project is a one button game called Diving Penguin. Diving Penguin is similar to Flappy Bird, but instead of a bird flying over pipes, this game will have a penguin diving under ice pillars. 

## Development Workflow
This will be a test driven development project. We will ONLY test to the requirements, NEVER testing implementation details of the code. We MUST be able to completely refactor the code with NONE of the tests breaking or changing.

We are using Claude to develop this project, so we must be able to emulate the code and visual output for Claude to inspect and run test on. We are planning to use LVGL + SDL for this purpose, with QEMU if needed for core logic.

## Hardware
The game will be implemented on an ESP32  m5stickc plus device. This device has documentation here: https://docs.m5stack.com/en/core/m5stickc_plus

The main components of the device we are interested in are the LCD screen: a 1.14 inch, 135 x 240 Colorful TFT LCD, ST7789v2: and button A, connected to GPIO37. This button is always referred to simply as the button.

## Key Gameplay Requirements

### Game Objective
- The goal of the game is to get as far as possible without the penguin contacting an ice pillar. When this happens, the score for how far the player made it is shown, and the game restarts.
- The game is lost if the player makes the penguin hit the edges of the screen or an ice pillar.
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
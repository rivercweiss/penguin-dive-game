#pragma once

#include <stdint.h>
#include <stdbool.h>

#define SCREEN_WIDTH 135
#define SCREEN_HEIGHT 240
#define PENGUIN_WIDTH 20
#define PENGUIN_HEIGHT 20

typedef struct {
    float x;
    float y;
    float velocity_y;
    float acceleration_y;
    bool button_pressed;
    bool was_button_pressed;
    uint32_t button_press_duration;
} penguin_t;

void penguin_physics_init(penguin_t* penguin);
void penguin_physics_update(penguin_t* penguin, bool button_pressed);
void penguin_physics_apply_dive_force(penguin_t* penguin, float force);
void penguin_physics_apply_rise_force(penguin_t* penguin);
bool penguin_physics_is_within_screen_bounds(penguin_t* penguin);
bool penguin_physics_is_at_screen_edge(penguin_t* penguin);
void penguin_physics_constrain_to_screen(penguin_t* penguin);
int penguin_physics_get_screen_x(penguin_t* penguin);
int penguin_physics_get_screen_y(penguin_t* penguin);
#include "penguin_physics.h"
#include <string.h>

#define GRAVITY 0.12f
#define DIVE_FORCE 6.0f
#define RISE_FORCE 1.5f
#define MAX_VELOCITY 3.5f
#define PENGUIN_START_X (SCREEN_WIDTH / 6.0f)
#define PENGUIN_START_Y (SCREEN_HEIGHT / 2.0f)

void penguin_physics_init(penguin_t* penguin) {
    if (!penguin) return;
    
    memset(penguin, 0, sizeof(penguin_t));
    penguin->x = PENGUIN_START_X;
    penguin->y = PENGUIN_START_Y;
    penguin->velocity_y = 0.0f;
    penguin->acceleration_y = GRAVITY;
}

void penguin_physics_update(penguin_t* penguin, bool button_pressed) {
    if (!penguin) return;
    
    // Track button state
    penguin->was_button_pressed = penguin->button_pressed;
    penguin->button_pressed = button_pressed;
    
    // Handle button press duration
    if (button_pressed) {
        penguin->button_press_duration++;
    } else {
        penguin->button_press_duration = 0;
    }
    
    // Apply forces based on button state
    if (button_pressed) {
        penguin_physics_apply_dive_force(penguin, DIVE_FORCE);
    } else {
        penguin_physics_apply_rise_force(penguin);
    }
    
    // Apply physics
    penguin->velocity_y += penguin->acceleration_y;
    // Add velocity damping for more control
    penguin->velocity_y *= 0.92f;
    
    // Clamp velocity
    if (penguin->velocity_y > MAX_VELOCITY) {
        penguin->velocity_y = MAX_VELOCITY;
    } else if (penguin->velocity_y < -MAX_VELOCITY) {
        penguin->velocity_y = -MAX_VELOCITY;
    }
    
    // Update position
    penguin->y += penguin->velocity_y;
    
    // Keep penguin within screen bounds
    penguin_physics_constrain_to_screen(penguin);
}

void penguin_physics_apply_dive_force(penguin_t* penguin, float force) {
    if (!penguin) return;
    
    // Diving force pulls penguin down - even less strong for easier control
    penguin->acceleration_y = GRAVITY + (force * 0.05f);
    
    // Immediate velocity boost for responsive controls - less strong
    if (!penguin->was_button_pressed && penguin->button_pressed) {
        penguin->velocity_y += force * 0.5f;
    }
}

void penguin_physics_apply_rise_force(penguin_t* penguin) {
    if (!penguin) return;
    
    // Rising force opposes gravity - even stronger for easier rising
    penguin->acceleration_y = -(RISE_FORCE - GRAVITY);
    
    // Much stronger upward velocity when button released for better control
    if (penguin->was_button_pressed && !penguin->button_pressed) {
        penguin->velocity_y -= RISE_FORCE * 4.0f;
    }
}

bool penguin_physics_is_within_screen_bounds(penguin_t* penguin) {
    if (!penguin) return false;
    
    return (penguin->y >= 0 && 
            penguin->y <= SCREEN_HEIGHT - PENGUIN_HEIGHT &&
            penguin->x >= 0 && 
            penguin->x <= SCREEN_WIDTH - PENGUIN_WIDTH);
}

bool penguin_physics_is_at_screen_edge(penguin_t* penguin) {
    if (!penguin) return false;
    
    // Check if penguin is beyond any screen edge (collision with edges)
    return (penguin->x < 0 || 
            penguin->y < 0 || 
            penguin->x + PENGUIN_WIDTH > SCREEN_WIDTH || 
            penguin->y + PENGUIN_HEIGHT > SCREEN_HEIGHT);
}

void penguin_physics_constrain_to_screen(penguin_t* penguin) {
    if (!penguin) return;
    
    // Constrain Y position
    if (penguin->y < 0) {
        penguin->y = 0;
        penguin->velocity_y = 0;
    } else if (penguin->y > SCREEN_HEIGHT - PENGUIN_HEIGHT) {
        penguin->y = SCREEN_HEIGHT - PENGUIN_HEIGHT;
        penguin->velocity_y = 0;
    }
    
    // Constrain X position (penguin should stay on left side)
    if (penguin->x < 0) {
        penguin->x = 0;
    } else if (penguin->x > SCREEN_WIDTH - PENGUIN_WIDTH) {
        penguin->x = SCREEN_WIDTH - PENGUIN_WIDTH;
    }
}

int penguin_physics_get_screen_x(penguin_t* penguin) {
    if (!penguin) return 0;
    return (int)penguin->x;
}

int penguin_physics_get_screen_y(penguin_t* penguin) {
    if (!penguin) return 0;
    return (int)penguin->y;
}
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize input subsystem
void input_init(void);

// Poll hardware/input state once per frame (call in main loop)
void input_poll(void);

// Returns true while the button is held down
bool input_button_pressed(void);

#ifdef __cplusplus
}
#endif

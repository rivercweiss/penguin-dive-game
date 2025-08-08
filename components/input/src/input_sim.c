#include "input.h"

static bool s_inited = false;
static bool s_pressed = false;

void input_init(void) {
    s_inited = true;
    s_pressed = false;
}

void input_poll(void) {
    (void)s_inited;
    // Simulator stub: no real input; remains false
}

bool input_button_pressed(void) {
    return s_pressed;
}

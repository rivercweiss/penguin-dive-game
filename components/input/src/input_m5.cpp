#include "input.h"

#ifdef ESP_PLATFORM
#include <M5Unified.h>

extern "C" {

static bool s_inited = false;

void input_init(void) {
    // Assume M5.begin() is already called by display driver; avoid double init.
    s_inited = true;
}

void input_poll(void) {
    if (!s_inited) return;
    M5.update();
}

bool input_button_pressed(void) {
    if (!s_inited) return false;
    return M5.BtnA.isPressed();
}

}
#endif

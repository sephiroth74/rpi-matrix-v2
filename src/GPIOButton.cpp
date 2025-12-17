#include "GPIOButton.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

GPIOButton::GPIOButton(int pin, int debounce_ms, int long_press_ms)
    : pin_(pin), debounce_ms_(debounce_ms), long_press_ms_(long_press_ms),
      last_value_(1), button_press_start_(0), button_was_pressed_(false),
      long_press_triggered_(false) {}

GPIOButton::~GPIOButton() {}

bool GPIOButton::setup() {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "pinctrl set %d ip pu", pin_);
    return system(cmd) == 0;
}

int GPIOButton::read() {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "pinctrl lev %d 2>/dev/null", pin_);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) return -1;

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        pclose(pipe);
        return (strchr(buffer, '1') != NULL) ? 1 : 0;
    }

    pclose(pipe);
    return -1;
}

void GPIOButton::poll(long current_time_ms) {
    int gpio_value = read();
    if (gpio_value < 0) return;

    // Detect falling edge (button pressed)
    if (last_value_ == 1 && gpio_value == 0) {
        button_press_start_ = current_time_ms;
        button_was_pressed_ = true;
        long_press_triggered_ = false;

        // Call onPress callback
        if (press_callback_) press_callback_();
    }
    // Check for long press while button is held
    else if (gpio_value == 0 && button_was_pressed_ && !long_press_triggered_) {
        long press_duration = current_time_ms - button_press_start_;

        if (press_duration >= long_press_ms_) {
            long_press_triggered_ = true;
            // Call onLongPress callback
            if (long_press_callback_) long_press_callback_();
        }
    }
    // Detect rising edge (button released)
    else if (last_value_ == 0 && gpio_value == 1 && button_was_pressed_) {
        long press_duration = current_time_ms - button_press_start_;

        // Call onRelease callback
        if (release_callback_) release_callback_();

        // Call onTap callback only if long press was not triggered
        if (!long_press_triggered_ && press_duration >= debounce_ms_) {
            if (tap_callback_) tap_callback_();
        }

        button_was_pressed_ = false;
        long_press_triggered_ = false;
    }

    last_value_ = gpio_value;
}

void GPIOButton::onPress(std::function<void()> callback) {
    press_callback_ = callback;
}

void GPIOButton::onRelease(std::function<void()> callback) {
    release_callback_ = callback;
}

void GPIOButton::onTap(std::function<void()> callback) {
    tap_callback_ = callback;
}

void GPIOButton::onLongPress(std::function<void()> callback) {
    long_press_callback_ = callback;
}

void GPIOButton::onShortPress(std::function<void()> callback) {
    // Alias for onTap for legacy compatibility
    tap_callback_ = callback;
}

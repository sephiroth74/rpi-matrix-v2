#ifndef GPIO_BUTTON_H
#define GPIO_BUTTON_H

#include <functional>

class GPIOButton {
public:
    GPIOButton(int pin, int debounce_ms = 100, int long_press_ms = 1000);
    ~GPIOButton();

    bool setup();
    void poll(long current_time_ms);

    // Callbacks
    void onShortPress(std::function<void()> callback);
    void onLongPress(std::function<void()> callback);

private:
    int pin_;
    int debounce_ms_;
    int long_press_ms_;
    int last_value_;
    long button_press_start_;
    bool button_was_pressed_;

    std::function<void()> short_press_callback_;
    std::function<void()> long_press_callback_;

    int read();
};

#endif // GPIO_BUTTON_H

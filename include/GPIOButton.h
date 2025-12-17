#ifndef GPIO_BUTTON_H
#define GPIO_BUTTON_H

#include <functional>

/**
 * GPIO Button Handler Class
 * Provides debounced button input with multiple event types:
 * - Press: Triggered when button is first pressed down
 * - Release: Triggered when button is released
 * - Tap: Triggered on short press (released before long_press_ms)
 * - Long Press: Triggered when button held for long_press_ms duration
 */
class GPIOButton {
public:
    /**
     * Constructor
     * @param pin GPIO pin number for button input
     * @param debounce_ms Debounce time in milliseconds (default: 80ms)
     * @param long_press_ms Long press threshold in milliseconds (default: 1000ms)
     */
    GPIOButton(int pin, int debounce_ms = 80, int long_press_ms = 1000);

    /**
     * Destructor
     */
    ~GPIOButton();

    /**
     * Setup GPIO pin with pull-up resistor
     * @return true if setup successful, false otherwise
     */
    bool setup();

    /**
     * Poll button state - call this regularly in main loop
     * @param current_time_ms Current time in milliseconds for timing calculations
     */
    void poll(long current_time_ms);

    // Event callback setters

    /**
     * Set callback for button press event
     * Called immediately when button is pressed down
     * @param callback Function to call on button press
     */
    void onPress(std::function<void()> callback);

    /**
     * Set callback for button release event
     * Called when button is released (regardless of press duration)
     * @param callback Function to call on button release
     */
    void onRelease(std::function<void()> callback);

    /**
     * Set callback for tap event
     * Called on short press (released before long_press_ms threshold)
     * @param callback Function to call on tap
     */
    void onTap(std::function<void()> callback);

    /**
     * Set callback for long press event
     * Called when button held for long_press_ms duration
     * Note: onTap will NOT be called if long press is triggered
     * @param callback Function to call on long press
     */
    void onLongPress(std::function<void()> callback);

    /**
     * Set callback for short press event (legacy compatibility)
     * Alias for onTap() - provided for backward compatibility
     * @param callback Function to call on short press
     */
    void onShortPress(std::function<void()> callback);

private:
    // Configuration
    int pin_;                   // GPIO pin number
    int debounce_ms_;           // Debounce time in milliseconds
    int long_press_ms_;         // Long press threshold in milliseconds

    // State tracking
    int last_value_;            // Last GPIO pin value (0 or 1)
    long button_press_start_;   // Timestamp when button was pressed
    bool button_was_pressed_;   // Flag indicating button is currently pressed
    bool long_press_triggered_; // Flag indicating long press was already triggered

    // Event callbacks
    std::function<void()> press_callback_;      // Called on button press
    std::function<void()> release_callback_;    // Called on button release
    std::function<void()> tap_callback_;        // Called on short press/tap
    std::function<void()> long_press_callback_; // Called on long press

    /**
     * Read current GPIO pin value
     * @return 0 (pressed), 1 (released), or -1 (error)
     */
    int read();
};

#endif // GPIO_BUTTON_H

#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <chrono>

/**
 * RGB Color structure
 * Simple structure to represent RGB color values
 */
struct RGBColor {
    uint8_t r, g, b;  // Red, Green, Blue components (0-255)

    /** Default constructor - initializes to black (0,0,0) */
    RGBColor() : r(0), g(0), b(0) {}

    /**
     * Constructor with RGB values
     * @param red Red component (0-255)
     * @param green Green component (0-255)
     * @param blue Blue component (0-255)
     */
    RGBColor(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
};

/**
 * Color Transition Animator
 * Provides smooth color transitions with cubic easing
 * Used for animated color changes in the LED matrix display
 */
class Animator {
public:
    /**
     * Constructor - initializes animator in idle state
     */
    Animator();

    /**
     * Start a new color transition animation
     * @param from Starting color
     * @param to Target color
     * @param durationMs Duration of transition in milliseconds
     */
    void startTransition(const RGBColor& from, const RGBColor& to, int durationMs);

    /**
     * Update animation state and get current interpolated color
     * Call this every frame to get the current animated color
     * @return Current interpolated color based on elapsed time
     */
    RGBColor update();

    /**
     * Check if animation is currently running
     * @return true if animating, false if idle or completed
     */
    bool isAnimating() const;

    /**
     * Cancel current animation and stop
     */
    void cancel();

private:
    /**
     * Cubic ease-in-out easing function
     * Provides smooth acceleration and deceleration
     * @param t Progress value (0.0 to 1.0)
     * @return Eased progress value (0.0 to 1.0)
     */
    double easeInOutCubic(double t) const;

    /**
     * Interpolate between two colors
     * Uses weighted average based on progress
     * @param from Starting color
     * @param to Target color
     * @param t Progress value (0.0 to 1.0)
     * @return Interpolated color
     */
    RGBColor interpolateColor(const RGBColor& from, const RGBColor& to, double t) const;

    // Animation state
    bool animating;                                    // True if animation is active
    RGBColor fromColor;                                // Starting color
    RGBColor toColor;                                  // Target color
    int durationMs;                                    // Total duration in milliseconds
    std::chrono::steady_clock::time_point startTime;   // Animation start timestamp
};

#endif // ANIMATOR_H

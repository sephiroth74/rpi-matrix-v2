#ifndef BORDER_SNAKE_ANIMATION_H
#define BORDER_SNAKE_ANIMATION_H

#include "Animator.h"
#include <vector>
#include <chrono>

/**
 * Point structure for 2D coordinates
 * Represents a pixel position on the LED matrix
 */
struct Point {
    int x, y;  // X and Y coordinates
    /**
     * Constructor
     * @param px X coordinate
     * @param py Y coordinate
     */
    Point(int px, int py) : x(px), y(py) {}
};

/**
 * Border Snake Animation
 * Creates an animated "snake" effect along the display border during color transitions
 * Two snakes start from the bottom-center and travel clockwise/counter-clockwise
 * to meet at the top-center, transitioning from one color to another
 */
class BorderSnakeAnimation {
public:
    /**
     * Constructor
     * @param width Display width in pixels
     * @param height Display height in pixels
     * @param maxSnakeLength Maximum length of each snake in pixels (default: 16)
     */
    BorderSnakeAnimation(int width, int height, int maxSnakeLength = 16);

    /**
     * Start a new snake animation synchronized with color transition
     * @param fromColor Starting color for the snake
     * @param toColor Ending color for the snake
     * @param durationMs Duration of animation in milliseconds
     */
    void start(const RGBColor& fromColor, const RGBColor& toColor, int durationMs);

    /**
     * Update animation state and get current frame
     * Call this every frame to get the list of pixels to draw
     * @return Vector of (Point, RGBColor) pairs representing pixels to draw
     */
    std::vector<std::pair<Point, RGBColor>> update();

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
     * Generate the border paths for the snakes
     * Creates two paths from bottom-center to top-center:
     * - Left path: counter-clockwise along border
     * - Right path: clockwise along border
     */
    void generateBorderPath();

    /**
     * Calculate snake positions at current time
     * @param progress Animation progress (0.0 to 1.0)
     * @param result Output vector for (Point, RGBColor) pairs
     */
    void calculateSnakePositions(double progress, std::vector<std::pair<Point, RGBColor>>& result);

    // Display dimensions
    int width;              // Display width in pixels
    int height;             // Display height in pixels
    int maxSnakeLength;     // Maximum length of each snake

    // Border paths (generated once at construction)
    std::vector<Point> pathLeft;   // Counter-clockwise: bottom-center -> left -> top-center
    std::vector<Point> pathRight;  // Clockwise: bottom-center -> right -> top-center

    // Animation state
    bool animating;                                    // True if animation is active
    Animator colorAnimator;                            // Color transition animator
    std::chrono::steady_clock::time_point startTime;   // Animation start timestamp
    int durationMs;                                    // Total duration in milliseconds
};

#endif // BORDER_SNAKE_ANIMATION_H

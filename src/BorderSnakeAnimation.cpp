#include "BorderSnakeAnimation.h"
#include <algorithm>
#include <cmath>

BorderSnakeAnimation::BorderSnakeAnimation(int w, int h, int maxLen)
    : width(w), height(h), maxSnakeLength(maxLen), animating(false), durationMs(0) {
    generateBorderPath();
}

void BorderSnakeAnimation::start(const RGBColor& fromColor, const RGBColor& toColor, int duration) {
    colorAnimator.startTransition(fromColor, toColor, duration);
    startTime = std::chrono::steady_clock::now();
    durationMs = duration;
    animating = true;
}

std::vector<std::pair<Point, RGBColor>> BorderSnakeAnimation::update() {
    std::vector<std::pair<Point, RGBColor>> result;

    if (!animating) {
        return result;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

    if (elapsed >= durationMs) {
        // Animation complete
        animating = false;
        return result;
    }

    // Calculate progress (0.0 to 1.0)
    double progress = static_cast<double>(elapsed) / durationMs;

    // Get current color from animator
    RGBColor currentColor = colorAnimator.update();

    // Calculate snake positions
    calculateSnakePositions(progress, result);

    // Apply color to all points
    for (auto& pair : result) {
        pair.second = currentColor;
    }

    return result;
}

bool BorderSnakeAnimation::isAnimating() const {
    return animating;
}

void BorderSnakeAnimation::cancel() {
    animating = false;
    colorAnimator.cancel();
}

void BorderSnakeAnimation::generateBorderPath() {
    // Clear existing paths
    pathLeft.clear();
    pathRight.clear();

    // Starting point: bottom-center
    int startX = width / 2;
    int startY = height - 1;

    // LEFT PATH (counter-clockwise): bottom-center -> left edge -> top-left -> top-center
    // 1. Go left along bottom edge
    for (int x = startX - 1; x >= 0; x--) {
        pathLeft.push_back(Point(x, startY));
    }
    // 2. Go up along left edge
    for (int y = startY - 1; y >= 0; y--) {
        pathLeft.push_back(Point(0, y));
    }
    // 3. Go right along top edge to center
    for (int x = 1; x <= width / 2; x++) {
        pathLeft.push_back(Point(x, 0));
    }

    // RIGHT PATH (clockwise): bottom-center -> right edge -> top-right -> top-center
    // 1. Go right along bottom edge
    for (int x = startX + 1; x < width; x++) {
        pathRight.push_back(Point(x, startY));
    }
    // 2. Go up along right edge
    for (int y = startY - 1; y >= 0; y--) {
        pathRight.push_back(Point(width - 1, y));
    }
    // 3. Go left along top edge to center
    for (int x = width - 2; x >= width / 2; x--) {
        pathRight.push_back(Point(x, 0));
    }
}

void BorderSnakeAnimation::calculateSnakePositions(double progress, std::vector<std::pair<Point, RGBColor>>& result) {
    // The snake expands from the start point and travels along the path
    // At progress = 0.0: snake is at start with length 1
    // At progress = 1.0: snake reaches the end (top-center) with full length

    // Total path length for each side
    int leftPathLen = pathLeft.size();
    int rightPathLen = pathRight.size();

    // Calculate how far along the path the snake head should be
    // The head travels the full path length during the animation
    double leftHeadPos = progress * (leftPathLen + maxSnakeLength);
    double rightHeadPos = progress * (rightPathLen + maxSnakeLength);

    // Draw left snake
    for (int i = 0; i < maxSnakeLength; i++) {
        int pathIndex = static_cast<int>(leftHeadPos) - i;
        if (pathIndex >= 0 && pathIndex < leftPathLen) {
            result.push_back(std::make_pair(pathLeft[pathIndex], RGBColor(0, 0, 0))); // Color will be set by caller
        }
    }

    // Draw right snake
    for (int i = 0; i < maxSnakeLength; i++) {
        int pathIndex = static_cast<int>(rightHeadPos) - i;
        if (pathIndex >= 0 && pathIndex < rightPathLen) {
            result.push_back(std::make_pair(pathRight[pathIndex], RGBColor(0, 0, 0))); // Color will be set by caller
        }
    }

    // Add starting point (bottom-center) at the beginning
    if (progress < 0.3) { // Show starting point during first 30% of animation
        result.push_back(std::make_pair(Point(width / 2, height - 1), RGBColor(0, 0, 0)));
    }
}

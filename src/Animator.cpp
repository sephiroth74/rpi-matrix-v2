#include "Animator.h"
#include <algorithm>
#include <cmath>

Animator::Animator() : animating(false), durationMs(0) {}

void Animator::startTransition(const RGBColor& from, const RGBColor& to, int duration) {
    fromColor = from;
    toColor = to;
    durationMs = duration;
    startTime = std::chrono::steady_clock::now();
    animating = true;
}

RGBColor Animator::update() {
    if (!animating) {
        return toColor;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

    if (elapsed >= durationMs) {
        // Transition complete
        animating = false;
        return toColor;
    }

    // Calculate progress (0.0 to 1.0)
    double progress = static_cast<double>(elapsed) / durationMs;

    // Apply easing and interpolate
    double easedProgress = easeInOutCubic(progress);
    return interpolateColor(fromColor, toColor, easedProgress);
}

bool Animator::isAnimating() const {
    return animating;
}

void Animator::cancel() {
    animating = false;
}

double Animator::easeInOutCubic(double t) const {
    return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
}

RGBColor Animator::interpolateColor(const RGBColor& from, const RGBColor& to, double t) const {
    RGBColor result;
    result.r = static_cast<uint8_t>(from.r * (1 - t) + to.r * t);
    result.g = static_cast<uint8_t>(from.g * (1 - t) + to.g * t);
    result.b = static_cast<uint8_t>(from.b * (1 - t) + to.b * t);
    return result;
}

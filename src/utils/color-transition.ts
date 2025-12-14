import type { Color } from "../module";
import type { ColorTransitionConfig } from "../config";

export class ColorTransition {
  private config: ColorTransitionConfig;
  private currentColorIndex: number = 0;
  private nextColorIndex: number = 1;
  private transitionStartTime: number = 0;
  private intervalStartTime: number = Date.now();
  private isTransitioning: boolean = false;

  constructor(config: ColorTransitionConfig) {
    this.config = config;
    if (config.colors.length > 0) {
      this.nextColorIndex = config.colors.length > 1 ? 1 : 0;
    }
  }

  /**
   * Get the current color, interpolated if in transition
   */
  getCurrentColor(): Color {
    if (!this.config.enabled || this.config.colors.length === 0) {
      return { r: 255, g: 255, b: 255 }; // Default white
    }

    if (this.config.colors.length === 1) {
      return this.config.colors[0];
    }

    const now = Date.now();
    const intervalMs = this.config.intervalMinutes * 60 * 1000;
    const transitionMs = this.config.transitionDurationSeconds * 1000;

    // Check if we should start a new transition
    if (!this.isTransitioning && now - this.intervalStartTime >= intervalMs) {
      this.startTransition();
    }

    // If not transitioning, return current color
    if (!this.isTransitioning) {
      return this.config.colors[this.currentColorIndex];
    }

    // Calculate transition progress (0 to 1)
    const elapsed = now - this.transitionStartTime;
    const progress = Math.min(elapsed / transitionMs, 1);

    // If transition complete, update indices
    if (progress >= 1) {
      this.isTransitioning = false;
      this.currentColorIndex = this.nextColorIndex;
      this.nextColorIndex = (this.nextColorIndex + 1) % this.config.colors.length;
      this.intervalStartTime = now;
      const finalColor = this.config.colors[this.currentColorIndex];
      console.log(
        `✓ Color transition complete → rgb(${finalColor.r}, ${finalColor.g}, ${finalColor.b})`,
      );
      return finalColor;
    }

    // Smooth easing function (ease-in-out)
    const easedProgress = this.easeInOutCubic(progress);

    // Interpolate between current and next color
    return this.interpolateColor(
      this.config.colors[this.currentColorIndex],
      this.config.colors[this.nextColorIndex],
      easedProgress,
    );
  }

  private startTransition(): void {
    this.isTransitioning = true;
    this.transitionStartTime = Date.now();
    const fromColor = this.config.colors[this.currentColorIndex];
    const toColor = this.config.colors[this.nextColorIndex];
    console.log(
      `→ Starting color transition: rgb(${fromColor.r}, ${fromColor.g}, ${fromColor.b}) → rgb(${toColor.r}, ${toColor.g}, ${toColor.b})`,
    );
  }

  /**
   * Smooth cubic easing function
   */
  private easeInOutCubic(t: number): number {
    return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
  }

  /**
   * Interpolate between two colors
   */
  private interpolateColor(color1: Color, color2: Color, t: number): Color {
    return {
      r: Math.round(color1.r + (color2.r - color1.r) * t),
      g: Math.round(color1.g + (color2.g - color1.g) * t),
      b: Math.round(color1.b + (color2.b - color1.b) * t),
    };
  }

  /**
   * Reset the transition state
   */
  reset(): void {
    this.currentColorIndex = 0;
    this.nextColorIndex = this.config.colors.length > 1 ? 1 : 0;
    this.intervalStartTime = Date.now();
    this.isTransitioning = false;
  }
}

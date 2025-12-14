import { readFileSync } from "node:fs";
import type { Color } from "./module";
import defaultConfig from "../config.json";

export interface ColorTransitionConfig {
  enabled: boolean;
  intervalMinutes: number;
  transitionDurationSeconds: number;
  colors: Color[];
}

export interface LetterClockConfig {
  dateColor: Color;
  timeColor: Color;
}

export interface AnalogClockConfig {
  hourHandColor: Color;
  minuteHandColor: Color;
  secondHandColor: Color;
  markersColor: Color;
  dateColor: Color;
}

export interface Config {
  brightness?: number; // Global brightness 0-100 (optional, defaults to 100)
  colorTransition: ColorTransitionConfig;
  letterClock: LetterClockConfig;
  analogClock: AnalogClockConfig;
}

/**
 * Try to load config from external file, fall back to bundled default
 */
export function loadConfig(): Config {
  // Try fonts directory first (same location as fonts, should work reliably)
  const externalPaths = [
    "/root/fonts/config.json",
    "/root/config.json",
    "/etc/clock-config.json",
  ];

  // Try to load from external file using Node.js fs (more reliable in compiled binaries)
  for (const path of externalPaths) {
    try {
      const text = readFileSync(path, "utf-8");
      const config = JSON.parse(text) as Config;
      console.log(`✓ Loaded external config from ${path}`);
      return config;
    } catch (error) {
      // File doesn't exist or can't be read, try next path
      console.log(`  Could not load ${path}: ${error}`);
      continue;
    }
  }

  // Fall back to bundled config
  console.log("Using bundled config.json (no external config found)");
  return defaultConfig as Config;
}

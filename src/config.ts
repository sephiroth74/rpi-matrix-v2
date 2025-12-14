import type { Color } from "./module";
import configData from "../config.json";

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
  letterClock: LetterClockConfig;
  analogClock: AnalogClockConfig;
}

export function loadConfig(): Config {
  console.log("Loading config from bundled config.json");
  return configData as Config;
}

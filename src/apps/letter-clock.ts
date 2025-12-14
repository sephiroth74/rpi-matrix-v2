import { Font, type LedMatrixInstance, type Color } from "../module";
import { wait } from "../utils";
import { loadConfig, type LetterClockConfig } from "../config";
import { ColorTransition } from "../utils/color-transition";

const bigFont = new Font("myFont", "/root/fonts/7x14B.bdf");
// const smallFont = new Font("smallFont", "./fonts/5x8.bdf");
const smallFont = new Font("smallFont", "/root/fonts/spleen-5x8.bdf");

const FONT_WIDTH = 7;
const FONT_HEIGHT = 14;

const SMALL_FONT_WIDTH = 5;
const SMALL_FONT_HEIGHT = 8;

const dayFormatter = new Intl.DateTimeFormat("it-IT", {
  weekday: "short",
  month: "short",
  day: "numeric",
});

const timeFormatter = new Intl.DateTimeFormat("it-IT", {
  hour: "2-digit",
  minute: "2-digit",
  second: "2-digit",
  hour12: false,
});

/**
 * Draws the time (HH:MM:SS) using big letters, centered on the panel.
 */
function drawTextClock(
  matrix: LedMatrixInstance,
  config: LetterClockConfig,
  transitionColor: Color | null,
) {
  matrix.clear();

  const dayLine = dayFormatter.format(new Date()).toUpperCase();
  const timeLine = timeFormatter.format(new Date());

  const dayLineWidth = dayLine.length * SMALL_FONT_WIDTH;
  const timeLineWidth = timeLine.length * FONT_WIDTH;

  const dayLineX = (matrix.width() - dayLineWidth) / 2;
  const timeLineX = (matrix.width() - timeLineWidth) / 2;
  const initialY = matrix.height() - (FONT_HEIGHT + SMALL_FONT_HEIGHT) - 5;

  // Use transition color if enabled, otherwise use static config color
  const color = transitionColor || config.timeColor;

  matrix.font(smallFont);
  matrix.fgColor(color);
  matrix.drawText(dayLine, dayLineX, initialY);

  matrix.font(bigFont);
  matrix.fgColor(color);
  matrix.drawText(timeLine, timeLineX, initialY + SMALL_FONT_HEIGHT);

  matrix.sync();
}

export const run = async (matrix: LedMatrixInstance) => {
  try {
    const config = loadConfig();
    const colorTransition = new ColorTransition(config.colorTransition);

    // Set brightness from config (default to 100 if not specified)
    const brightness = config.brightness ?? 100;
    matrix.brightness(brightness);
    console.log(`✓ Brightness set to ${brightness}%`);

    while (true) {
      const transitionColor = config.colorTransition.enabled
        ? colorTransition.getCurrentColor()
        : null;

      drawTextClock(matrix, config.letterClock, transitionColor);
      await wait(1000); // update once per second
    }
  } catch (error) {
    console.error(`${__filename} caught: `, error);
  }
};

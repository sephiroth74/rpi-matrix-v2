#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

// Hardware and system configuration
#define GPIO_NUM 19                         // GPIO pin number for button input
#define CONFIG_PATH "/root/clock-config.json"  // Path to configuration file

// Display timing constants
#define COLOR_DISPLAY_MS 2000               // Duration to show color/brightness messages (ms)
#define VERSION_DISPLAY_MS 4000             // Duration to show version/IP at startup (ms)

// Brightness control constants
#define MIN_BRIGHTNESS 20                   // Minimum brightness level (%)
#define MAX_BRIGHTNESS 100                  // Maximum brightness level (%)
#define BRIGHTNESS_INC_STEP 10              // Brightness increment step (%)

// Main loop timing
#define MAIN_LOOP_USLEEP 30000              // Main loop sleep duration (microseconds, 40ms = 25 FPS)

/**
 * Named color structure for display colors
 */
struct NamedColor {
    std::string name;  // Display name shown when cycling colors (e.g., "ROSSO", "BLU")
    uint8_t r, g, b;   // RGB color components (0-255)
};

/**
 * Configuration class for LED Matrix Clock
 * Manages loading, saving, and validating all clock settings
 */
class Config {
public:
    // Display settings
    int brightness;                         // Current brightness level (MIN_BRIGHTNESS to MAX_BRIGHTNESS)
    int fixed_color;                        // Fixed color index (-1 = AUTO mode, 0+ = specific color)
    std::vector<NamedColor> colors;         // Available color palette

    // Color transition settings
    bool colorTransitionEnabled;            // Enable automatic color transitions in AUTO mode
    int colorTransitionIntervalMinutes;     // Minutes between automatic color changes
    int colorTransitionDurationMs;          // Duration of color transition animation (ms)

    // Time and date formatting
    std::string dateFormat;                 // strftime format string for date (e.g., "%a %d %b")
    std::string timeFormat;                 // strftime format string for time (e.g., "%H:%M")

    // Display visibility
    bool showDate;                          // Show/hide date display
    bool showTime;                          // Show/hide time display

    // Font settings
    std::string dateFont;                   // BDF font file for date (e.g., "5x8.bdf")
    std::string timeFont;                   // BDF font file for time (e.g., "7x14B.bdf")
    bool dateIgnoreDescenders;              // Ignore descenders for date (true for uppercase-only text)
    bool timeIgnoreDescenders;              // Ignore descenders for time (true for uppercase-only text)
    int dateTimeSpacing;                    // Vertical spacing between date and time in pixels

    /**
     * Constructor - initializes configuration with default values
     */
    Config();

    /**
     * Load configuration from JSON file
     * @param path Path to JSON configuration file
     * @return true if loaded successfully, false otherwise
     */
    bool load(const char* path);

    /**
     * Save current configuration to JSON file
     * @param path Path to JSON configuration file
     * @return true if saved successfully, false otherwise
     */
    bool save(const char* path);
};

#endif // CONFIG_H

// LED Matrix Clock in C++ with Config, Color Selection, and Brightness Control
// Based on hzeller/rpi-rgb-led-matrix examples

#include "led-matrix.h"
#include "graphics.h"
#include "version.h"

#include <unistd.h>
#include <ctime>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <vector>

using namespace rgb_matrix;

#define GPIO_NUM 19
#define DEBOUNCE_MS 100
#define LONG_PRESS_MS 1000
#define CONFIG_PATH "/root/config-simple.json"
#define COLOR_DISPLAY_MS 2000
#define VERSION_DISPLAY_MS 3000

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

// Simple color structure
struct NamedColor {
    std::string name;
    uint8_t r, g, b;
};

// Configuration structure
struct Config {
    int brightness;
    int fixed_color;
    std::vector<NamedColor> colors;
    bool colorTransitionEnabled;
};

// GPIO helper functions using pinctrl
bool gpio_setup(int pin) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "pinctrl set %d ip pu", pin);
    return system(cmd) == 0;
}

int gpio_read(int pin) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "pinctrl lev %d 2>/dev/null", pin);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) return -1;

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        pclose(pipe);
        // Output format: "1" or "0"
        return (strchr(buffer, '1') != NULL) ? 1 : 0;
    }

    pclose(pipe);
    return -1;
}

// Simple JSON parser for our config
bool loadConfig(const char* path, Config& config) {
    std::ifstream file(path);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open config file: %s\n", path);
        return false;
    }

    // Read entire file
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    // Parse brightness
    size_t pos = content.find("\"brightness\"");
    if (pos != std::string::npos) {
        size_t colon = content.find(":", pos);
        size_t comma = content.find_first_of(",}", colon);
        std::string value = content.substr(colon + 1, comma - colon - 1);
        config.brightness = std::stoi(value);
    }

    // Parse fixed_color
    pos = content.find("\"fixed_color\"");
    if (pos != std::string::npos) {
        size_t colon = content.find(":", pos);
        size_t comma = content.find_first_of(",}", colon);
        std::string value = content.substr(colon + 1, comma - colon - 1);
        config.fixed_color = std::stoi(value);
    }

    // Parse colors array
    config.colors.clear();
    pos = content.find("\"colors\"");
    if (pos != std::string::npos) {
        size_t arrayStart = content.find("[", pos);
        size_t arrayEnd = content.find("]", arrayStart);
        std::string colorsArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

        size_t objStart = 0;
        while ((objStart = colorsArray.find("{", objStart)) != std::string::npos) {
            size_t objEnd = colorsArray.find("}", objStart);
            std::string colorObj = colorsArray.substr(objStart, objEnd - objStart + 1);

            NamedColor nc;

            // Parse name
            size_t namePos = colorObj.find("\"name\"");
            if (namePos != std::string::npos) {
                size_t nameStart = colorObj.find("\"", namePos + 6) + 1;
                size_t nameEnd = colorObj.find("\"", nameStart);
                nc.name = colorObj.substr(nameStart, nameEnd - nameStart);
            }

            // Parse r
            size_t rPos = colorObj.find("\"r\"");
            if (rPos != std::string::npos) {
                size_t colon = colorObj.find(":", rPos);
                size_t comma = colorObj.find_first_of(",}", colon);
                nc.r = std::stoi(colorObj.substr(colon + 1, comma - colon - 1));
            }

            // Parse g
            size_t gPos = colorObj.find("\"g\"");
            if (gPos != std::string::npos) {
                size_t colon = colorObj.find(":", gPos);
                size_t comma = colorObj.find_first_of(",}", colon);
                nc.g = std::stoi(colorObj.substr(colon + 1, comma - colon - 1));
            }

            // Parse b
            size_t bPos = colorObj.find("\"b\"");
            if (bPos != std::string::npos) {
                size_t colon = colorObj.find(":", bPos);
                size_t comma = colorObj.find_first_of(",}", colon);
                nc.b = std::stoi(colorObj.substr(colon + 1, comma - colon - 1));
            }

            config.colors.push_back(nc);
            objStart = objEnd + 1;
        }
    }

    // Parse colorTransition.enabled
    pos = content.find("\"enabled\"");
    if (pos != std::string::npos) {
        size_t colon = content.find(":", pos);
        std::string value = content.substr(colon + 1, 10);
        config.colorTransitionEnabled = (value.find("true") != std::string::npos);
    }

    return true;
}

// Save config
bool saveConfig(const char* path, const Config& config) {
    FILE* file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open config file for writing: %s\n", path);
        return false;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"brightness\": %d,\n", config.brightness);
    fprintf(file, "  \"fixed_color\": %d,\n", config.fixed_color);
    fprintf(file, "  \"colors\": [\n");

    for (size_t i = 0; i < config.colors.size(); i++) {
        const NamedColor& nc = config.colors[i];
        fprintf(file, "    { \"name\": \"%s\", \"r\": %d, \"g\": %d, \"b\": %d }%s\n",
                nc.name.c_str(), nc.r, nc.g, nc.b,
                (i < config.colors.size() - 1) ? "," : "");
    }

    fprintf(file, "  ],\n");
    fprintf(file, "  \"colorTransition\": {\n");
    fprintf(file, "    \"enabled\": %s,\n", config.colorTransitionEnabled ? "true" : "false");
    fprintf(file, "    \"intervalMinutes\": 120,\n");
    fprintf(file, "    \"transitionDurationSeconds\": 30\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    fclose(file);
    return true;
}

int main(int argc, char *argv[]) {
    // Print version
    printf("═══════════════════════════════════════\n");
    printf("  LED Matrix Clock v%s\n", VERSION_STRING);
    printf("═══════════════════════════════════════\n\n");

    // Load config
    Config config;
    if (!loadConfig(CONFIG_PATH, config)) {
        fprintf(stderr, "Using default config\n");
        config.brightness = 50;
        config.fixed_color = -1;
        config.colors = {
            {"GIALLO", 255, 220, 0},
            {"ROSSO", 255, 0, 0},
            {"VERDE", 0, 255, 0},
            {"BLU", 0, 0, 255},
            {"BIANCO", 255, 255, 255}
        };
        config.colorTransitionEnabled = true;
    }

    printf("✓ Config loaded: brightness=%d, fixed_color=%d, colors=%zu\n",
           config.brightness, config.fixed_color, config.colors.size());

    // Load fonts
    rgb_matrix::Font font_large;
    const char *font_large_path = "/root/fonts/7x14B.bdf";
    if (!font_large.LoadFont(font_large_path)) {
        fprintf(stderr, "Couldn't load large font: %s\n", font_large_path);
        return 1;
    }

    rgb_matrix::Font font_small;
    const char *font_small_path = "/root/fonts/spleen-5x8.bdf";
    if (!font_small.LoadFont(font_small_path)) {
        fprintf(stderr, "Couldn't load small font: %s\n", font_small_path);
        return 1;
    }

    // Matrix configuration
    RGBMatrix::Options matrix_options;
    RuntimeOptions runtime_opt;

    matrix_options.rows = 32;
    matrix_options.cols = 64;
    matrix_options.chain_length = 1;
    matrix_options.parallel = 1;
    matrix_options.hardware_mapping = "adafruit-hat";
    matrix_options.led_rgb_sequence = "RBG";
    runtime_opt.gpio_slowdown = 4;
    runtime_opt.drop_privileges = 0; // Don't drop privileges - we need root for config file writes
    matrix_options.brightness = config.brightness;

    // Create matrix
    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (matrix == NULL) {
        fprintf(stderr, "Failed to create matrix\n");
        return 1;
    }

    printf("✓ Matrix initialized\n");

    // Setup signal handler
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // Setup GPIO button
    int last_gpio_value = 1;
    long button_press_start = 0;
    bool button_was_pressed = false;

    if (!gpio_setup(GPIO_NUM)) {
        fprintf(stderr, "Failed to setup GPIO %d\n", GPIO_NUM);
        return 1;
    }
    printf("✓ GPIO %d configured with pull-up\n", GPIO_NUM);

    // Create canvases (for double buffering and text measurement)
    FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
    FrameCanvas *temp_canvas = matrix->CreateFrameCanvas();

    // Get current time for version display
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long startup_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    // Message display state - show version at startup
    long message_display_until = startup_time + VERSION_DISPLAY_MS;
    std::string message_text = "v" + std::string(VERSION_STRING);
    Color message_color(255, 255, 255);

    printf("Clock started.\n");
    printf("  Short press: Cycle brightness (10%% - 100%%)\n");
    printf("  Long press: Cycle colors / AUTO mode\n");

    while (!interrupt_received) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        long current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

        // Check button state
        int gpio_value = gpio_read(GPIO_NUM);
        if (gpio_value >= 0) {
            // Detect falling edge (button pressed)
            if (last_gpio_value == 1 && gpio_value == 0) {
                button_press_start = current_time;
                button_was_pressed = true;
            }
            // Detect rising edge (button released)
            else if (last_gpio_value == 0 && gpio_value == 1 && button_was_pressed) {
                long press_duration = current_time - button_press_start;

                if (press_duration >= LONG_PRESS_MS) {
                    // Long press: cycle colors
                    config.fixed_color++;
                    if (config.fixed_color >= (int)config.colors.size()) {
                        config.fixed_color = -1; // Back to AUTO
                        message_text = "AUTO";
                        message_color = Color(255, 255, 255); // White for AUTO
                        config.colorTransitionEnabled = true;
                    } else {
                        const NamedColor& nc = config.colors[config.fixed_color];
                        message_text = nc.name;
                        message_color = Color(nc.r, nc.g, nc.b); // Use the selected color
                        config.colorTransitionEnabled = false;
                    }

                    message_display_until = current_time + COLOR_DISPLAY_MS;
                    saveConfig(CONFIG_PATH, config);
                    printf("🎨 Color: %s\n", message_text.c_str());
                }
                else if (press_duration >= DEBOUNCE_MS) {
                    // Short press: cycle brightness
                    config.brightness += 10;
                    if (config.brightness > 100) config.brightness = 10;

                    matrix->SetBrightness(config.brightness);
                    saveConfig(CONFIG_PATH, config);

                    // Show brightness message
                    char brightness_msg[16];
                    snprintf(brightness_msg, sizeof(brightness_msg), "%d%%", config.brightness);
                    message_text = brightness_msg;
                    message_color = Color(255, 255, 255); // White for brightness
                    message_display_until = current_time + COLOR_DISPLAY_MS;

                    printf("💡 Brightness: %d%%\n", config.brightness);
                }

                button_was_pressed = false;
            }
            last_gpio_value = gpio_value;
        }

        // Clear canvas
        offscreen_canvas->Clear();

        // Determine current color and display
        Color display_color;
        if (current_time < message_display_until) {
            // Display message (color name or brightness)
            int width = DrawText(temp_canvas, font_large, 0, 0, message_color, NULL, message_text.c_str());
            int x = (64 - width) / 2;
            int y = 20;

            DrawText(offscreen_canvas, font_large, x, y, message_color, NULL, message_text.c_str());
        } else {
            // Normal clock display
            if (config.fixed_color >= 0 && config.fixed_color < (int)config.colors.size()) {
                const NamedColor& nc = config.colors[config.fixed_color];
                display_color = Color(nc.r, nc.g, nc.b);
            } else {
                // AUTO mode - use default yellow
                display_color = Color(255, 220, 0);
            }

            // Get current time
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);

            // Format date
            char date_buffer[32];
            const char* day_names[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};
            const char* month_names[] = {"GEN", "FEB", "MAR", "APR", "MAG", "GIU",
                                          "LUG", "AGO", "SET", "OTT", "NOV", "DIC"};
            snprintf(date_buffer, sizeof(date_buffer), "%s %d %s",
                     day_names[tm_info->tm_wday],
                     tm_info->tm_mday,
                     month_names[tm_info->tm_mon]);

            // Format time
            char time_buffer[16];
            strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);

            // Measure text widths
            int date_width = DrawText(temp_canvas, font_small, 0, 0, display_color, NULL, date_buffer);
            int time_width = DrawText(temp_canvas, font_large, 0, 0, display_color, NULL, time_buffer);

            // Get font heights from BDF metadata
            int time_height = font_large.height();
            int date_height = font_small.height();

            // Calculate centered positions
            const int MATRIX_WIDTH = 64;
            const int MATRIX_HEIGHT = 32;
            const int SPACING = 1; // Space between date and time

            // Total content height
            int total_height = date_height + SPACING + time_height;

            // Center vertically
            int start_y = (MATRIX_HEIGHT - total_height) / 2;

            // Calculate X positions (horizontal centering)
            int date_x = (MATRIX_WIDTH - date_width) / 2;
            int time_x = (MATRIX_WIDTH - time_width) / 2;

            // Calculate Y positions (baselines)
            int date_y = start_y + font_small.baseline();
            int time_y = date_y + date_height - font_small.baseline() + SPACING + font_large.baseline();

            // Draw date and time
            DrawText(offscreen_canvas, font_small, date_x, date_y, display_color, NULL, date_buffer);
            DrawText(offscreen_canvas, font_large, time_x, time_y, display_color, NULL, time_buffer);
        }

        // Swap buffers
        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

        // Update frequency
        usleep(50000); // 50ms for responsive button detection
    }

    // Cleanup
    matrix->Clear();
    delete matrix;

    printf("\nClock stopped.\n");
    return 0;
}

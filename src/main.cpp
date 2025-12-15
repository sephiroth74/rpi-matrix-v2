// LED Matrix Clock in C++ with Config, Color Selection, and Brightness Control
// Based on hzeller/rpi-rgb-led-matrix examples

#include "led-matrix.h"
#include "graphics.h"
#include "version.h"
#include "Config.h"
#include "GPIOButton.h"

// Include locale file based on LOCALE_FILE define (set in Makefile)
#ifndef LOCALE_FILE
#define LOCALE_FILE "locale/en_US.h"
#endif
#include LOCALE_FILE

#include <unistd.h>
#include <ctime>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <string>

using namespace rgb_matrix;

#define GPIO_NUM 19
#define CONFIG_PATH "/root/clock-config.json"
#define COLOR_DISPLAY_MS 2000
#define VERSION_DISPLAY_MS 3000

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

// Global state for button callbacks
Config* g_config = nullptr;
RGBMatrix* g_matrix = nullptr;
long* g_message_display_until = nullptr;
std::string* g_message_text = nullptr;
Color* g_message_color = nullptr;

// Get current time in milliseconds
long getCurrentTimeMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Short press callback: cycle brightness
void onShortPress() {
    g_config->brightness += 10;
    if (g_config->brightness > 100) g_config->brightness = 10;

    g_matrix->SetBrightness(g_config->brightness);
    g_config->save(CONFIG_PATH);

    // Show brightness message
    char brightness_msg[16];
    snprintf(brightness_msg, sizeof(brightness_msg), "%d%%", g_config->brightness);
    *g_message_text = brightness_msg;
    *g_message_color = Color(255, 255, 255); // White for brightness
    *g_message_display_until = getCurrentTimeMs() + COLOR_DISPLAY_MS;

    printf("💡 Brightness: %d%%\n", g_config->brightness);
}

// Long press callback: cycle colors
void onLongPress() {
    g_config->fixed_color++;
    if (g_config->fixed_color >= (int)g_config->colors.size()) {
        g_config->fixed_color = -1; // Back to AUTO
        *g_message_text = Locale::MSG_AUTO;
        *g_message_color = Color(255, 255, 255); // White for AUTO
        g_config->colorTransitionEnabled = true;
    } else {
        const NamedColor& nc = g_config->colors[g_config->fixed_color];
        *g_message_text = nc.name;
        *g_message_color = Color(nc.r, nc.g, nc.b); // Use the selected color
        g_config->colorTransitionEnabled = false;
    }

    *g_message_display_until = getCurrentTimeMs() + COLOR_DISPLAY_MS;
    g_config->save(CONFIG_PATH);
    printf("🎨 Color: %s\n", g_message_text->c_str());
}

int main(int argc, char *argv[]) {
    // Print version
    printf("═══════════════════════════════════════\n");
    printf("  LED Matrix Clock v%s\n", VERSION_STRING);
    printf("═══════════════════════════════════════\n\n");

    // Load config using Config class
    Config config;
    bool config_loaded = config.load(CONFIG_PATH);

    // Print current configuration
    if (config_loaded) {
        printf("✓ Configuration loaded from %s\n", CONFIG_PATH);
    } else {
        printf("⚠ Failed to load %s, using default configuration\n", CONFIG_PATH);
    }
    printf("  Brightness: %d%%\n", config.brightness);
    printf("  Fixed color: %d ", config.fixed_color);
    if (config.fixed_color == -1) {
        printf("(AUTO mode)\n");
    } else if (config.fixed_color >= 0 && config.fixed_color < (int)config.colors.size()) {
        const NamedColor& nc = config.colors[config.fixed_color];
        printf("(%s - RGB(%d, %d, %d))\n", nc.name.c_str(), nc.r, nc.g, nc.b);
    } else {
        printf("(invalid)\n");
    }
    printf("  Color transition: %s\n", config.colorTransitionEnabled ? "enabled" : "disabled");
    printf("  Available colors: %zu\n", config.colors.size());
    for (size_t i = 0; i < config.colors.size(); i++) {
        const NamedColor& nc = config.colors[i];
        printf("    %zu. %s - RGB(%d, %d, %d)\n", i, nc.name.c_str(), nc.r, nc.g, nc.b);
    }
    printf("\n");

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

    // Create canvases (for double buffering and text measurement)
    FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
    FrameCanvas *temp_canvas = matrix->CreateFrameCanvas();

    // Message display state - show version at startup
    long startup_time = getCurrentTimeMs();
    long message_display_until = startup_time + VERSION_DISPLAY_MS;
    std::string message_text = std::string(Locale::MSG_VERSION_PREFIX) + std::string(VERSION_STRING);
    Color message_color(255, 255, 255);

    // Setup global pointers for button callbacks
    g_config = &config;
    g_matrix = matrix;
    g_message_display_until = &message_display_until;
    g_message_text = &message_text;
    g_message_color = &message_color;

    // Setup GPIO button using GPIOButton class
    GPIOButton button(GPIO_NUM);
    if (!button.setup()) {
        fprintf(stderr, "Failed to setup GPIO %d\n", GPIO_NUM);
        return 1;
    }
    button.onShortPress(onShortPress);
    button.onLongPress(onLongPress);
    printf("✓ GPIO %d configured with pull-up\n", GPIO_NUM);

    printf("Clock started.\n");
    printf("  Short press: Cycle brightness (10%% - 100%%)\n");
    printf("  Long press: Cycle colors / AUTO mode\n");

    while (!interrupt_received) {
        long current_time = getCurrentTimeMs();

        // Poll button for press events
        button.poll(current_time);

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

            // Format date using locale
            char date_buffer[32];
            snprintf(date_buffer, sizeof(date_buffer), Locale::DATE_FORMAT,
                     Locale::DAY_NAMES[tm_info->tm_wday],
                     tm_info->tm_mday,
                     Locale::MONTH_NAMES[tm_info->tm_mon]);

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

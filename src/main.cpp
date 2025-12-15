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
#include <cmath>
#include <locale.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

using namespace rgb_matrix;

#define GPIO_NUM 19
#define CONFIG_PATH "/root/clock-config.json"
#define COLOR_DISPLAY_MS 2000
#define VERSION_DISPLAY_MS 5000

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

// Get local IP address
std::string getLocalIP() {
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];
    std::string result = "No IP";

    if (getifaddrs(&ifaddr) == -1) {
        return result;
    }

    // Look for non-loopback IPv4 address
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

            // Skip loopback (127.0.0.1)
            if (strcmp(ip, "127.0.0.1") != 0) {
                result = ip;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return result;
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

// Ease-in-out cubic function for smooth transitions
float easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - (float)pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

// Interpolate between two colors
Color interpolateColor(const NamedColor& from, const NamedColor& to, float progress) {
    float eased = easeInOutCubic(progress);
    uint8_t r = (uint8_t)(from.r * (1.0f - eased) + to.r * eased);
    uint8_t g = (uint8_t)(from.g * (1.0f - eased) + to.g * eased);
    uint8_t b = (uint8_t)(from.b * (1.0f - eased) + to.b * eased);
    return Color(r, g, b);
}

int main(int argc, char *argv[]) {
    // Set locale for date/time formatting (from Makefile LOCALE variable)
#ifdef SYSTEM_LOCALE
    setlocale(LC_TIME, SYSTEM_LOCALE);
#else
    setlocale(LC_TIME, "it_IT.UTF-8");
#endif

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
    if (config.colorTransitionEnabled) {
        printf("    Interval: %d minutes\n", config.colorTransitionIntervalMinutes);
        printf("    Duration: %d ms\n", config.colorTransitionDurationMs);
    }
    printf("  Date format: \"%s\"\n", config.dateFormat.c_str());
    printf("  Time format: \"%s\"\n", config.timeFormat.c_str());
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
    const char *font_small_path = "/root/fonts/5x8.bdf";
    if (!font_small.LoadFont(font_small_path)) {
        fprintf(stderr, "Couldn't load small font: %s\n", font_small_path);
        return 1;
    }

    rgb_matrix::Font font_tiny;
    const char *font_tiny_path = "/root/fonts/4x6.bdf";
    if (!font_tiny.LoadFont(font_tiny_path)) {
        fprintf(stderr, "Couldn't load tiny font: %s\n", font_tiny_path);
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

    // Get local IP address
    std::string local_ip = getLocalIP();
    printf("🌐 Local IP: %s\n", local_ip.c_str());

    // Display IP and version at startup
    long startup_time = getCurrentTimeMs();
    long message_display_until = startup_time + VERSION_DISPLAY_MS;
    Color startup_color(255, 255, 255);

    // Show IP and version for a few seconds
    offscreen_canvas->Clear();

    // Draw IP address in tiny font (centered)
    int ip_width = DrawText(temp_canvas, font_tiny, 0, 0, startup_color, NULL, local_ip.c_str());
    int ip_x = (64 - ip_width) / 2;
    int ip_y = 12; // Upper half
    DrawText(offscreen_canvas, font_tiny, ip_x, ip_y, startup_color, NULL, local_ip.c_str());

    // Draw version in small font below (centered)
    std::string version_text = std::string(Locale::MSG_VERSION_PREFIX) + std::string(VERSION_STRING);
    int version_width = DrawText(temp_canvas, font_small, 0, 0, startup_color, NULL, version_text.c_str());
    int version_x = (64 - version_width) / 2;
    int version_y = 26; // Lower half
    DrawText(offscreen_canvas, font_small, version_x, version_y, startup_color, NULL, version_text.c_str());

    offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

    // Wait for display duration
    usleep(VERSION_DISPLAY_MS * 1000);

    // Reset message state for normal operation
    std::string message_text = "";
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

    // Color transition state
    int current_color_index = 0;
    int next_color_index = 1;
    long transition_start_time = getCurrentTimeMs();
    long intervalMs = config.colorTransitionIntervalMinutes * 60 * 1000; // Convert minutes to ms
    long next_color_change_time = transition_start_time + intervalMs;

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
                // Fixed color mode
                const NamedColor& nc = config.colors[config.fixed_color];
                display_color = Color(nc.r, nc.g, nc.b);
            } else if (config.colorTransitionEnabled && config.colors.size() >= 2) {
                // AUTO mode with smooth transitions
                long time_until_next_change = next_color_change_time - current_time;

                // Check if we're in the transition window (last N milliseconds before color change)
                if (time_until_next_change <= config.colorTransitionDurationMs && time_until_next_change > 0) {
                    // Transitioning to next color
                    float progress = 1.0f - ((float)time_until_next_change / (float)config.colorTransitionDurationMs);
                    display_color = interpolateColor(
                        config.colors[current_color_index],
                        config.colors[next_color_index],
                        progress
                    );
                } else if (time_until_next_change <= 0) {
                    // Time to switch to next color
                    current_color_index = next_color_index;
                    next_color_index = (next_color_index + 1) % config.colors.size();
                    next_color_change_time = current_time + intervalMs;

                    // Display the new current color
                    const NamedColor& nc = config.colors[current_color_index];
                    display_color = Color(nc.r, nc.g, nc.b);

                    // Debug log
                    printf("🔄 Color changed to %s RGB(%d,%d,%d), next in %dmin\n",
                           nc.name.c_str(), nc.r, nc.g, nc.b, config.colorTransitionIntervalMinutes);
                } else {
                    // Not in transition - display current color
                    const NamedColor& nc = config.colors[current_color_index];
                    display_color = Color(nc.r, nc.g, nc.b);
                }
            } else {
                // Fallback - use first color or yellow
                if (config.colors.size() > 0) {
                    const NamedColor& nc = config.colors[0];
                    display_color = Color(nc.r, nc.g, nc.b);
                } else {
                    display_color = Color(255, 220, 0);
                }
            }

            // Get current time
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);

            // Format date and time using config formats
            char date_buffer[32];
            char time_buffer[16];
            strftime(date_buffer, sizeof(date_buffer), config.dateFormat.c_str(), tm_info);
            strftime(time_buffer, sizeof(time_buffer), config.timeFormat.c_str(), tm_info);

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
        usleep(40000); // 50ms for responsive button detection
    }

    // Cleanup
    matrix->Clear();
    delete matrix;

    printf("\nClock stopped.\n");
    return 0;
}

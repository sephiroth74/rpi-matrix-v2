// LED Matrix Clock in C++ with Config, Color Selection, and Brightness Control
// Based on hzeller/rpi-rgb-led-matrix examples

#include "led-matrix.h"
#include "graphics.h"
#include "version.h"
#include "Config.h"
#include "GPIOButton.h"
#include "Animator.h"
#include "BorderSnakeAnimation.h"

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
#include <vector>
#include <sstream>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int) {
    interrupt_received = true;
}

// Global state for button callbacks
Config* g_config = nullptr;
RGBMatrix* g_matrix = nullptr;
long* g_message_display_until = nullptr;
std::string* g_message_text = nullptr;
Color* g_message_color = nullptr;
Animator* g_animator = nullptr;
BorderSnakeAnimation* g_snakeAnimation = nullptr;
bool g_showing_auto_transition = false;

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
    g_config->brightness += BRIGHTNESS_INC_STEP;
    if (g_config->brightness > MAX_BRIGHTNESS) g_config->brightness = MIN_BRIGHTNESS;

    g_matrix->SetBrightness(g_config->brightness);
    g_config->save(CONFIG_PATH);

    // Show brightness message
    char brightness_msg[16];
    snprintf(brightness_msg, sizeof(brightness_msg), "%d%%", g_config->brightness);
    *g_message_text = brightness_msg;

    // Color warning for brightness > 100%
    if (g_config->brightness > 100) {
        *g_message_color = Color(255, 100, 0); // Orange warning for high brightness
    } else {
        *g_message_color = Color(255, 255, 255); // White for normal brightness
    }
    *g_message_display_until = getCurrentTimeMs() + COLOR_DISPLAY_MS;

    printf("💡 Brightness: %d%%\n", g_config->brightness);
}

// Long press callback: cycle colors
void onLongPress() {
    // Get current color before changing
    RGBColor fromColor;
    if (g_config->fixed_color >= 0 && g_config->fixed_color < (int)g_config->colors.size()) {
        const NamedColor& nc = g_config->colors[g_config->fixed_color];
        fromColor = RGBColor(nc.r, nc.g, nc.b);
    } else if (g_animator && g_animator->isAnimating()) {
        // If animating, get the current animated color
        fromColor = g_animator->update();
    } else if (g_config->colors.size() > 0) {
        // Fallback to first color
        const NamedColor& nc = g_config->colors[0];
        fromColor = RGBColor(nc.r, nc.g, nc.b);
    } else {
        fromColor = RGBColor(255, 220, 0);
    }

    // Cycle to next color
    g_config->fixed_color++;
    if (g_config->fixed_color >= (int)g_config->colors.size()) {
        g_config->fixed_color = -1; // Back to AUTO
        *g_message_text = Locale::MSG_AUTO;
        g_config->colorTransitionEnabled = true;
        g_showing_auto_transition = true; // Flag to show AUTO message during transition
    } else {
        const NamedColor& nc = g_config->colors[g_config->fixed_color];
        *g_message_text = nc.name;
        g_config->colorTransitionEnabled = false;
        g_showing_auto_transition = false;
    }

    // Start transition animation to new color
    RGBColor toColor;
    if (g_config->fixed_color >= 0 && g_config->fixed_color < (int)g_config->colors.size()) {
        const NamedColor& nc = g_config->colors[g_config->fixed_color];
        toColor = RGBColor(nc.r, nc.g, nc.b);
    } else if (g_config->colors.size() > 0) {
        // AUTO mode - start with first color
        const NamedColor& nc = g_config->colors[0];
        toColor = RGBColor(nc.r, nc.g, nc.b);
    } else {
        toColor = RGBColor(255, 220, 0);
    }

    // Start the transition with configured duration
    if (g_animator) {
        g_animator->startTransition(fromColor, toColor, g_config->colorTransitionDurationMs);
    }

    // Start the border snake animation synchronized with color transition
    if (g_snakeAnimation) {
        g_snakeAnimation->start(fromColor, toColor, g_config->colorTransitionDurationMs);
    }

    *g_message_display_until = 0; // No text message - just show the transition
    g_config->save(CONFIG_PATH);
    printf("🎨 Color: %s\n", g_message_text->c_str());
}


int main(int, char**) {
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

    // Validate and clamp brightness
    if (config.brightness < MIN_BRIGHTNESS) {
        printf("⚠ Brightness %d%% below minimum, setting to %d%%\n", config.brightness, MIN_BRIGHTNESS);
        config.brightness = MIN_BRIGHTNESS;
        config.save(CONFIG_PATH);
    } else if (config.brightness > MAX_BRIGHTNESS) {
        printf("⚠ Brightness %d%% above maximum, clamping to %d%%\n", config.brightness, MAX_BRIGHTNESS);
        config.brightness = MAX_BRIGHTNESS;
        config.save(CONFIG_PATH);
    }

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
    // Date and time fonts from config with fallback to defaults
    std::string date_font_path = "/root/fonts/" + config.dateFont;
    std::string time_font_path = "/root/fonts/" + config.timeFont;

    rgb_matrix::Font font_date;
    if (!font_date.LoadFont(date_font_path.c_str())) {
        fprintf(stderr, "⚠ Couldn't load date font: %s, using default 5x8.bdf\n", date_font_path.c_str());
        if (!font_date.LoadFont("/root/fonts/5x8.bdf")) {
            fprintf(stderr, "❌ Failed to load default date font\n");
            return 1;
        }
    }

    rgb_matrix::Font font_time;
    if (!font_time.LoadFont(time_font_path.c_str())) {
        fprintf(stderr, "⚠ Couldn't load time font: %s, using default 7x14B.bdf\n", time_font_path.c_str());
        if (!font_time.LoadFont("/root/fonts/7x14B.bdf")) {
            fprintf(stderr, "❌ Failed to load default time font\n");
            return 1;
        }
    }

    // Tiny font for IP display (always 4x6.bdf)
    rgb_matrix::Font font_tiny;
    const char *font_tiny_path = "/root/fonts/4x6.bdf";
    if (!font_tiny.LoadFont(font_tiny_path)) {
        fprintf(stderr, "Couldn't load tiny font: %s\n", font_tiny_path);
        return 1;
    }

    // For message display, use larger of the two fonts
    rgb_matrix::Font* font_message = font_time.height() >= font_date.height() ? &font_time : &font_date;

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

    // Draw version in date font below (centered)
    std::string version_text = std::string(Locale::MSG_VERSION_PREFIX) + std::string(VERSION_STRING);
    int version_width = DrawText(temp_canvas, font_date, 0, 0, startup_color, NULL, version_text.c_str());
    int version_x = (64 - version_width) / 2;
    int version_y = 26; // Lower half
    DrawText(offscreen_canvas, font_date, version_x, version_y, startup_color, NULL, version_text.c_str());

    offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

    // Wait for display duration
    usleep(VERSION_DISPLAY_MS * 1000);

    // Reset message state for normal operation
    std::string message_text = "";
    Color message_color(255, 255, 255);

    // Create Animator instance
    Animator animator;

    // Create BorderSnakeAnimation instance (64x32 display, 16 pixel snake length)
    BorderSnakeAnimation snakeAnimation(64, 32, 16);

    // Setup global pointers for button callbacks
    g_config = &config;
    g_matrix = matrix;
    g_message_display_until = &message_display_until;
    g_message_text = &message_text;
    g_message_color = &message_color;
    g_animator = &animator;
    g_snakeAnimation = &snakeAnimation;

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
    printf("  Short press: Cycle brightness (%d%% - %d%%)\n", MIN_BRIGHTNESS, MAX_BRIGHTNESS);
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
            int width = DrawText(temp_canvas, *font_message, 0, 0, message_color, NULL, message_text.c_str());
            int x = (64 - width) / 2;
            int y = 20;

            DrawText(offscreen_canvas, *font_message, x, y, message_color, NULL, message_text.c_str());
        } else if (g_showing_auto_transition && animator.isAnimating()) {
            // Show AUTO message during transition to AUTO mode
            RGBColor rgb = animator.update();
            display_color = Color(rgb.r, rgb.g, rgb.b);

            int width = DrawText(temp_canvas, *font_message, 0, 0, display_color, NULL, Locale::MSG_AUTO);
            int x = (64 - width) / 2;
            int y = 20;

            DrawText(offscreen_canvas, *font_message, x, y, display_color, NULL, Locale::MSG_AUTO);
        } else {
            // Normal clock display

            // Reset AUTO transition flag when animation is done
            if (g_showing_auto_transition && !animator.isAnimating()) {
                g_showing_auto_transition = false;
            }

            // Check if there's an active manual transition from button press
            if (animator.isAnimating()) {
                // Use animator's current color during manual transition
                RGBColor rgb = animator.update();
                display_color = Color(rgb.r, rgb.g, rgb.b);
            } else if (config.fixed_color >= 0 && config.fixed_color < (int)config.colors.size()) {
                // Fixed color mode (no animation)
                const NamedColor& nc = config.colors[config.fixed_color];
                display_color = Color(nc.r, nc.g, nc.b);
            } else if (config.colorTransitionEnabled && config.colors.size() >= 2) {
                // AUTO mode with smooth transitions
                long time_until_next_change = next_color_change_time - current_time;

                // Check if we're in the transition window (last N milliseconds before color change)
                if (time_until_next_change <= config.colorTransitionDurationMs && time_until_next_change > 0) {
                    // Start transitioning to next color if not already animating
                    if (!animator.isAnimating()) {
                        const NamedColor& from = config.colors[current_color_index];
                        const NamedColor& to = config.colors[next_color_index];
                        animator.startTransition(
                            RGBColor(from.r, from.g, from.b),
                            RGBColor(to.r, to.g, to.b),
                            config.colorTransitionDurationMs
                        );
                    }
                    RGBColor rgb = animator.update();
                    display_color = Color(rgb.r, rgb.g, rgb.b);
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

            // Convert date to uppercase
            for (size_t i = 0; i < strlen(date_buffer); i++) {
                date_buffer[i] = toupper(date_buffer[i]);
            }

            // Calculate centered positions
            const int MATRIX_WIDTH = 64;
            const int MATRIX_HEIGHT = 32;

            // Conditional rendering based on config
            if (config.showDate && config.showTime) {
                // Show both date and time
                int date_width = DrawText(temp_canvas, font_date, 0, 0, display_color, NULL, date_buffer);
                int time_width = DrawText(temp_canvas, font_time, 0, 0, display_color, NULL, time_buffer);

                // Get font metrics
                int date_height = font_date.height();
                int time_height = font_time.height();
                int date_baseline = font_date.baseline();
                int time_baseline = font_time.baseline();

                // Calculate visual heights based on ignoreDescenders flags
                // If ignoring descenders (for uppercase/numbers only), use only ascent
                // Otherwise use full font height
                int date_visual_height = config.dateIgnoreDescenders ? date_baseline : date_height;
                int time_visual_height = config.timeIgnoreDescenders ? time_baseline : time_height;

                // Use configured spacing, but clamp if needed to fit on display
                int spacing = config.dateTimeSpacing;
                int total_height = date_visual_height + spacing + time_visual_height;
                if (total_height > MATRIX_HEIGHT) {
                    // Clamp spacing to fit
                    spacing = MATRIX_HEIGHT - date_visual_height - time_visual_height;
                    if (spacing < 0) spacing = 0;  // Minimum spacing
                    total_height = date_visual_height + spacing + time_visual_height;
                }

                // Center the visible content vertically
                int start_y = (MATRIX_HEIGHT - total_height) / 2;

                // Calculate X positions (horizontal centering)
                int date_x = (MATRIX_WIDTH - date_width) / 2;
                int time_x = (MATRIX_WIDTH - time_width) / 2;

                // Calculate Y positions (baseline positions for DrawText)
                int date_y = start_y + date_baseline;
                int time_y = start_y + date_visual_height + spacing + time_baseline;

                // Draw date and time
                DrawText(offscreen_canvas, font_date, date_x, date_y, display_color, NULL, date_buffer);
                DrawText(offscreen_canvas, font_time, time_x, time_y, display_color, NULL, time_buffer);
            } else if (config.showDate && !config.showTime) {
                // Show only date (centered vertically, with word wrap if needed)
                int date_width = DrawText(temp_canvas, font_date, 0, 0, display_color, NULL, date_buffer);

                if (date_width <= MATRIX_WIDTH) {
                    // Date fits in one line - center it
                    int date_x = (MATRIX_WIDTH - date_width) / 2;
                    int date_y = (MATRIX_HEIGHT / 2) + (font_date.baseline() / 2);
                    DrawText(offscreen_canvas, font_date, date_x, date_y, display_color, NULL, date_buffer);
                } else {
                    // Date too wide - split into words and wrap
                    std::vector<std::string> lines;
                    std::string current_line = "";
                    std::string date_str(date_buffer);
                    std::istringstream words(date_str);
                    std::string word;

                    while (words >> word) {
                        std::string test_line = current_line.empty() ? word : current_line + " " + word;
                        int test_width = DrawText(temp_canvas, font_date, 0, 0, display_color, NULL, test_line.c_str());

                        if (test_width <= MATRIX_WIDTH) {
                            current_line = test_line;
                        } else {
                            if (!current_line.empty()) {
                                lines.push_back(current_line);
                            }
                            current_line = word;
                        }
                    }
                    if (!current_line.empty()) {
                        lines.push_back(current_line);
                    }

                    // Calculate total height and center vertically
                    int date_height = font_date.height();
                    int total_height = lines.size() * date_height;
                    int start_y = (MATRIX_HEIGHT - total_height) / 2;

                    // Draw each line centered
                    for (size_t i = 0; i < lines.size(); i++) {
                        int line_width = DrawText(temp_canvas, font_date, 0, 0, display_color, NULL, lines[i].c_str());
                        int line_x = (MATRIX_WIDTH - line_width) / 2;
                        int line_y = start_y + (i * date_height) + font_date.baseline();
                        DrawText(offscreen_canvas, font_date, line_x, line_y, display_color, NULL, lines[i].c_str());
                    }
                }
            } else if (!config.showDate && config.showTime) {
                // Show only time (centered vertically)
                int time_width = DrawText(temp_canvas, font_time, 0, 0, display_color, NULL, time_buffer);

                // Center horizontally and vertically
                int time_x = (MATRIX_WIDTH - time_width) / 2;
                int time_y = (MATRIX_HEIGHT / 2) + (font_time.baseline() / 2);

                DrawText(offscreen_canvas, font_time, time_x, time_y, display_color, NULL, time_buffer);
            }
            // If neither is shown (shouldn't happen due to validation), nothing is drawn
        }

        // Draw border snake animation if active (on top of everything)
        if (snakeAnimation.isAnimating()) {
            auto snakePixels = snakeAnimation.update();
            for (const auto& pixel : snakePixels) {
                const Point& p = pixel.first;
                const RGBColor& c = pixel.second;
                offscreen_canvas->SetPixel(p.x, p.y, c.r, c.g, c.b);
            }
        }

        // Swap buffers
        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

        // Update frequency
        usleep(MAIN_LOOP_USLEEP); // 50ms for responsive button detection
    }

    // Cleanup
    matrix->Clear();
    delete matrix;

    printf("\nClock stopped.\n");
    return 0;
}

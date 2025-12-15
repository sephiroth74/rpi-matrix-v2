#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct NamedColor {
    std::string name;
    uint8_t r, g, b;
};

class Config {
public:
    int brightness;
    int fixed_color;
    std::vector<NamedColor> colors;
    bool colorTransitionEnabled;
    int colorTransitionIntervalMinutes; // Minutes between color changes
    int colorTransitionDurationMs;       // Milliseconds for transition animation
    std::string dateFormat;
    std::string timeFormat;

    Config();
    bool load(const char* path);
    bool save(const char* path);
};

#endif // CONFIG_H

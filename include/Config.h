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

    Config();
    bool load(const char* path);
    bool save(const char* path);
};

#endif // CONFIG_H

#include "Config.h"
#include "json.hpp"
#include <fstream>
#include <cstdio>

using json = nlohmann::json;

Config::Config() : brightness(50), fixed_color(-1), colorTransitionEnabled(true) {
    // Default colors
    colors = {
        {"GIALLO", 255, 220, 0},
        {"ROSSO", 255, 0, 0},
        {"VERDE", 0, 255, 0},
        {"BLU", 0, 0, 255},
        {"BIANCO", 255, 255, 255}
    };
}

bool Config::load(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open config file: %s\n", path);
        return false;
    }

    try {
        json j;
        file >> j;

        // Load values
        if (j.contains("brightness")) brightness = j["brightness"];
        if (j.contains("fixed_color")) fixed_color = j["fixed_color"];

        // Load colors
        if (j.contains("colors") && j["colors"].is_array()) {
            colors.clear();
            for (const auto& color : j["colors"]) {
                NamedColor nc;
                nc.name = color["name"];
                nc.r = color["r"];
                nc.g = color["g"];
                nc.b = color["b"];
                colors.push_back(nc);
            }
        }

        // Load colorTransition
        if (j.contains("colorTransition") && j["colorTransition"].contains("enabled")) {
            colorTransitionEnabled = j["colorTransition"]["enabled"];
        }

        return true;
    } catch (const std::exception& e) {
        fprintf(stderr, "Error parsing config: %s\n", e.what());
        return false;
    }
}

bool Config::save(const char* path) {
    try {
        json j;
        j["brightness"] = brightness;
        j["fixed_color"] = fixed_color;

        // Save colors
        j["colors"] = json::array();
        for (const auto& nc : colors) {
            json color;
            color["name"] = nc.name;
            color["r"] = nc.r;
            color["g"] = nc.g;
            color["b"] = nc.b;
            j["colors"].push_back(color);
        }

        // Save colorTransition
        j["colorTransition"]["enabled"] = colorTransitionEnabled;
        j["colorTransition"]["intervalMinutes"] = 120;
        j["colorTransition"]["transitionDurationSeconds"] = 30;

        // Write to file
        std::ofstream file(path);
        if (!file.is_open()) {
            fprintf(stderr, "Failed to open config file for writing: %s\n", path);
            return false;
        }

        file << j.dump(2); // Pretty print with 2 spaces
        return true;
    } catch (const std::exception& e) {
        fprintf(stderr, "Error saving config: %s\n", e.what());
        return false;
    }
}

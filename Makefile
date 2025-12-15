# LED Matrix Clock Makefile

# Compiler and flags
CXX = g++
CXXFLAGS = -O3 -Wall -Wextra -std=c++11
INCLUDES = -I/root/rpi-rgb-led-matrix/include
LDFLAGS = -L/root/rpi-rgb-led-matrix/lib
LIBS = /root/rpi-rgb-led-matrix/lib/librgbmatrix.a -lrt -lm -lpthread -lstdc++

# Directories
SRC_DIR = src
BUILD_DIR = build
CONFIG_DIR = config

# Target
TARGET = $(BUILD_DIR)/led-clock

# Source files
SOURCES = $(SRC_DIR)/main.cpp
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Link
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo "Build complete: $(TARGET)"

# Clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# Install (copy to /root and restart service)
install: $(TARGET)
	cp $(TARGET) /root/clock-full
	cp $(CONFIG_DIR)/config-simple.json /root/config-simple.json
	systemctl restart led-clock.service
	@echo "Installation complete"

# Status
status:
	systemctl status led-clock.service --no-pager

# Logs
logs:
	journalctl -u led-clock.service -f

.PHONY: all clean install status logs

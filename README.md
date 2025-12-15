# 3D Printed Raspberry Pi Matrix Clock

> **Note**: This is a C++ rewrite of the [original TypeScript/Bun version](https://github.com/alessandrobattisti/rpi-matrix-clock). The C++ version offers better performance, lower memory footprint, and native integration with the LED matrix and GPIO library.

![](assets/final.jpeg)

## Hardware

- [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/)
- [Adafruit 64x32 RGB LED Matrix - 2.5mm pitch](https://www.adafruit.com/product/5036)
    - Can be replaced by any 64x32 Hub75 2.5mm pitch led matrix, they usually are the same dimensions and work the same way as the Adafruit one. Check Amazon or AliExpress.
- [Adafruit RGB Matrix Bonnet for Raspberry Pi](https://www.adafruit.com/product/3211)
- [Tobsun DC 12V/24V to 5V 10A Converter](https://www.amazon.com/VOLRANTISE-Converter-Voltage-Regulator-Transformer/dp/B09WZ9DC9W)
- [USB-C Decoy Fast Charge Trigger with screw terminal](https://www.amazon.com/AITRIP-Charging-Trigger-Detector-Terminal/dp/B098WPSMV9)
- [5.5mm x 2.1mm Barrel Jack Adapter](https://www.amazon.com/CENTROPOWER-Connectors-Pigtail-Adapter-Security/dp/B0BTHSDF4S)
- M3x6 Machine Screws (10 pcs)
- M2x4 Machine Screws (4 pcs)
- M3 Heated inserts (4 pcs)
- M2 Heated inserts (4 pcs)
- Black vinyl sticker (50-80% transparency)
- USB-C to USB-C cable
- 40W USB-C Power adapter

## 3D Printed Case

1. Print body and bottom plate using a 0.4 Nozzle and 0.2mm layer height in PLA
  - I recommend cutting the model in your slicer and printing it with a snug support.
  - If you've cut the model like me, use super glue to attach them back together.
2. Print diffuser using 0.2 Nozzle and 0.1mm layer height in PLA
  - Print the bottom 2 layers in white PLA (diffuser)
  - Print the top 20 layers in black PLA (grid)
  - ⚠️ The grid has a single 0.2mm wall so it's not possible to print with a 0.4 Nozzle.

  ![](assets/diffuser.jpeg)

## Assembly

1. Insert 4 M3 heated inserts using a soldering iron in designated spots in the main body.
  ![](assets/bottom.png)
2. Insert 4 M2 heated inserts in desginated spots for Raspberry Pi on the bottom plate.
  ![](assets/bottom-plate.png)
3. Press the diffuser to the front of the matrix display. It should snap in place with a little bit of force.
  - Optionally stick a black vinyl sticker to the white part to make the screen dark.
4. Attach the matrix display to the front using 4 M3 machine screws.
  ![](assets/front-plate.png)
5. Attach the Raspberry Pi to the mounting holes using 4 M2 screws and then attach the bonnet to it.
6. Screw the buck converter in place using 2 M3 screws from the bottom.
  ![](assets/hardware.png)
7. Set the output of the USB-C PD trigger to 20V according to it's instructions, and connect 2 wires from it's terminals to the buck converter's input
8. Connect a pigtail barrel jack adapter to the buck converter's output, and plug in the other end to the bonnet.
9. Connect bonnet's data and power cable to the LED Matrix according to manufacturer's instructions.
10. Carefully slide in the USB-C PD trigger into the designated spot on the back of the model.
11. Screw the bottom plate in using 4 M3 machine screws.
  ![](assets/final.png)

## Software

- I recommend installing [DietPi](https://dietpi.com) instead of the official OS. It's much faster.
- [Prepare your RPI for headless installation](https://dietpi.com/docs/install/) and remote ssh.

### Features

- **Centered display** with customizable date/time format (e.g., MON 15 DEC + HH:MM:SS)
- **Brightness control** via physical button (GPIO 19)
  - **Short press (< 1s)**: Cycle brightness in 10% steps (10% → 20% → ... → 100% → 10%)
  - Briefly displays "XX%" on screen when brightness changes
- **Color selection** via physical button
  - **Long press (≥ 1s)**: Cycle through 10 customizable colors + AUTO mode
  - Color name is displayed for 2 seconds in the selected color
  - AUTO mode: smooth color transitions with configurable timing
- **Smooth color transitions** in AUTO mode
  - Uses cubic ease-in-out interpolation for natural color changes
  - Configurable interval (minutes) and transition duration (milliseconds)
- **Persistent configuration** saved to `/root/clock-config.json`
  - Brightness and selected color are saved automatically
  - Changes persist after reboot
- **Systemd service** for automatic startup
- **Startup display** shows local IP address and version for 5 seconds
  - Useful for SSH access without connecting a monitor
- **Multi-language support** via locale files (Italian and English included)
  - Both compile-time (UI messages) and runtime (date/time formatting)

### Available Colors

The clock supports 10 predefined colors + AUTO mode:

1. **ROSSO** (255, 0, 0) - Pure red
2. **ARANCIO** (255, 128, 0) - Orange
3. **GIALLO** (255, 220, 0) - Yellow (default in AUTO)
4. **VERDE** (0, 255, 0) - Pure green
5. **CIANO** (0, 255, 255) - Cyan
6. **BLU** (0, 0, 255) - Pure blue
7. **AZZURRO** (173, 216, 255) - Light blue
8. **VIOLA** (128, 0, 255) - Purple
9. **MAGENTA** (255, 0, 255) - Magenta
10. **BIANCO** (255, 255, 255) - White
11. **AUTO** - Automatic transition between all colors (fixed_color = -1)

### Configuration

The `/root/clock-config.json` file contains all settings:

```json
{
  "brightness": 50,              // Current brightness (10-100)
  "fixed_color": -1,             // Color index (-1 = AUTO, 0-9 = fixed color)
  "colors": [                    // Array of available colors
    {
      "name": "ROSSO",           // Name shown on display
      "r": 255,                  // Red component (0-255)
      "g": 0,                    // Green component (0-255)
      "b": 0                     // Blue component (0-255)
    },
    // ... other colors
  ],
  "colorTransition": {
    "enabled": true,             // Enable smooth transitions in AUTO mode
    "intervalMinutes": 2,        // Minutes between color changes
    "durationMs": 1000           // Transition duration in milliseconds
  },
  "dateFormat": "%a %d %b",      // Date format (strftime)
  "timeFormat": "%H:%M:%S"       // Time format (strftime)
```

**How to modify colors:**

1. Edit `/root/clock-config.json` on the Raspberry Pi
2. Modify the RGB values (0-255) for each color
3. You can add/remove colors from the array
4. Restart the service: `systemctl restart led-clock.service`

**How to change default brightness:**

1. Modify the `"brightness"` value in the config (10-100)
2. Or use the button and the value will be saved automatically

**How to adjust color transitions:**

1. Edit `colorTransition.intervalMinutes` - how long each color is displayed (in minutes)
2. Edit `colorTransition.durationMs` - how long the transition animation lasts (in milliseconds)
3. Example: `intervalMinutes: 2` with `durationMs: 1000` means each color is shown for 2 minutes, with a 1-second smooth transition to the next color

**How to customize date/time format:**

1. Edit `dateFormat` and `timeFormat` using [strftime format codes](https://man7.org/linux/man-pages/man3/strftime.3.html)
2. Examples:
   - `"%a %d %b"` → "Mon 15 Dec"
   - `"%d/%m/%Y"` → "15/12/2024"
   - `"%H:%M:%S"` → "16:34:42"
   - `"%I:%M %p"` → "04:34 PM"

### Installation

1. Clone this repository on your Raspberry Pi
2. Install dependencies:
```bash
# Install rpi-rgb-led-matrix library
cd /root
git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
cd rpi-rgb-led-matrix
make

# Install build tools if not present
apt-get install -y g++ make
```

3. Build and install the clock:
```bash
cd /path/to/rpi-matrix-v2
make
make install
```

4. The clock will start automatically on boot. You can manage it with:
```bash
systemctl status led-clock.service   # Check status
systemctl restart led-clock.service  # Restart
journalctl -u led-clock.service -f   # View logs
```

### Localization

The clock supports multiple languages through locale files. By default, it uses Italian (it_IT).

**Available locales:**
- `it_IT` - Italian (default)
- `en_US` - English

**How to change the language:**

1. Open the Makefile and set the `LOCALE` variable:
```makefile
LOCALE = en_US
```

2. Rebuild the project:
```bash
make clean
make
make install
```

Alternatively, you can build with a specific locale without editing the Makefile:
```bash
make LOCALE=en_US
```

**What changes with locale:**
- Day names (e.g., DOM → SUN, LUN → MON)
- Month names (e.g., GEN → JAN, DIC → DEC)
- UI messages (e.g., AUTO, version prefix)

**Adding a new locale:**

1. Create a new file in `locale/` (e.g., `locale/fr_FR.h`)
2. Copy the structure from `locale/en_US.h`
3. Translate the day names, month names, and messages
4. Build with: `make LOCALE=fr_FR`

### Button Usage

- **GPIO 19** connected to a momentary push button (pull-up enabled internally)
- **Short press**: Increase brightness (10% steps)
- **Long press**: Cycle through colors or return to AUTO mode

### Useful Commands

**Service management:**
```bash
# Check service status
systemctl status led-clock.service

# Restart the service
systemctl restart led-clock.service

# Stop the service
systemctl stop led-clock.service

# Start the service
systemctl start led-clock.service

# View logs in real-time
journalctl -u led-clock.service -f

# View recent logs
journalctl -u led-clock.service -n 50
```

**Rebuild and update:**
```bash
cd /root/rpi-matrix-v2
make clean && make LOCALE=it_IT
systemctl stop led-clock.service
cp build/led-clock /root/clock-full
systemctl start led-clock.service
```

**Update configuration:**
```bash
# Edit config file
nano /root/clock-config.json

# Restart to apply changes
systemctl restart led-clock.service
```

## CAD Files

STEP and STL files are available in `cad` directory.

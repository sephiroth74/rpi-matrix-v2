# 3D Printed Raspberry Pi Matrix Clock

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

- **Display centrato** con formato italiano (DOM 15 DIC + HH:MM:SS)
- **Controllo brightness** con pulsante fisico (GPIO 19)
  - **Short press (< 1s)**: Cambia luminosità 10% → 100%
- **Selezione colori** con pulsante fisico
  - **Long press (≥ 1s)**: Cicla tra 10 colori + modalità AUTO
  - Il nome del colore viene mostrato brevemente nel colore selezionato
- **Configurazione persistente** salvata in `/root/config-simple.json`
- **Service systemd** per avvio automatico

### Colori disponibili

1. ROSSO, 2. ARANCIO, 3. GIALLO, 4. VERDE, 5. CIANO, 6. BLU, 7. AZZURRO, 8. VIOLA, 9. MAGENTA, 10. BIANCO, 11. AUTO (transizione automatica)

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

### Button Usage

- **GPIO 19** connected to a momentary push button (pull-up enabled internally)
- **Short press**: Increase brightness (10% steps)
- **Long press**: Cycle through colors or return to AUTO mode

## CAD Files

STEP and STL files are available in `cad` directory.

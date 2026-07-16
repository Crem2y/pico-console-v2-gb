# Pico Console V2 GB

![Front view](doc/front.jpg)

[![Build test](https://github.com/Crem2y/pico-console-v2-gb/actions/workflows/build_test.yml/badge.svg)](https://github.com/Crem2y/pico-console-v2-gb/actions/workflows/build_test.yml)

A Game Boy emulator for the Pico Console V2 platform.

Audio is handled by an external RP2350-based Link-APU over a custom communication protocol.

## Features

- microSD ROM loading
- Save RAM (`.sav`) support
- PSRAM-backed ROM storage
- 2x display scaling
- External Link-APU audio processing
- Hardware gamepad input
- Built-in ROM file browser

### Controls

| Action              | Joypad              |
|---------------------|---------------------|
| A or select file    | A                   |
| B                   | B                   |
| Start               | START               |
| Select              | SELECT              |
| D-Pad               | Left Stick or D-Pad |
| Volume Up           | R                   |
| Volume Down         | L                   |
| 2x Scaling (toggle) | RS                  |
| Frame Skip (Toggle) | LS + ZL             |
| Interlace (Toggle)  | LS + ZR             |
| Capture BMP         | SUB1                |
| Save & Exit         | SUB2                |

## How to build & upload firmware

1. Install CMake (at least version 3.13), Python 3, and a GCC cross compiler
```bash
sudo apt install cmake python3 build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```
2. Clone this repository with submodules:
```bash
git clone --recurse-submodules https://github.com/Crem2y/pico-console-v2-gb.git
```
3. Launch the build script:
```bash
./pico_build.sh
```
4. Press the reset button twice to enter bootloader mode.

5. Upload the generated `.uf2` file to your board.
    - if you already installed [picotool](https://github.com/raspberrypi/picotool), use this.
```bash
./pico_upload.sh
```

6. (Optional) Launch the clean script to remove build artifacts:
```bash
./pico_clean.sh
```

## Photos
- working!

## Schematics & PCB
- See [pico-console-v2-pcb](https://github.com/Crem2y/pico-console-v2-pcb) for details.

---

## License
- This project is licensed under the MIT License.  
- See [LICENSE](./LICENSE) for details.

---

### Credits

This project is derived from Pico-GB, which itself is based on RP2040-GB and Peanut-GB.

```
Peanut-GB
    ↓
RP2040-GB
    ↓
Pico-GB
    ↓
Pico Console V2 GB
```

### Third-party components
- Thank you to the many open-source contributors.
- See the `third_party_licenses/` directory for full details.
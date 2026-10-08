# Pico Console V2 GB

![Front view](doc/front.jpg)

[![Build test](https://github.com/Crem2y/pico-console-v2-gb/actions/workflows/build_test.yml/badge.svg)](https://github.com/Crem2y/pico-console-v2-gb/actions/workflows/build_test.yml)

A Game Boy emulator for the [Pico Console V2](https://github.com/Crem2y/pico-console-v2) handheld platform.

This project adapts the Pico-GB emulator to the custom hardware and firmware environment of Pico Console V2, including external audio processing through an RP2350-based Link-APU.

## Features

- Game Boy ROM loading from microSD
- Save RAM (`.sav`) support
- PSRAM-backed ROM storage
- Built-in ROM file browser
- 2x display scaling
- External Link-APU audio processing
- Hardware gamepad input
- Frame skipping and interlace mode
- BMP screenshot capture

## Implementation

- **Platform**: Pico Console V2
- **Emulator lineage**: Peanut-GB → RP2040-GB → Pico-GB
- **ROM storage**: External PSRAM
- **Audio**: External RP2350-based Link-APU using a custom communication protocol
- **Display**: Native and 2x scaling modes

## Controls

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

## Hardware

This application runs on the Pico Console V2 platform.

- [Pico Console V2 Firmware](https://github.com/Crem2y/pico-console-v2)
- [RP2350A Main Board](https://github.com/Crem2y/rp2350a_main_board)
- [Pico Console V2 PCB](https://github.com/Crem2y/pico-console-v2-pcb)

## License
- This project is licensed under the MIT License.  
- See [LICENSE](./LICENSE) for details.

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
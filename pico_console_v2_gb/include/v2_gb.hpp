#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// pico libraries
#include "pico/stdlib.h"
#include "pico/multicore.h"

// common
#include "common.h"

// drivers
#include "uart_rx.h"
#include "uart_tx.h"
#include "uart_log.h"
#include "uart_bridge.h"
#include "led_builtin.h"
#include "led_status.hpp"
#include "psram_apsxx04.h"
#include "ili9488_40_hstx.hpp"
#include "system_time.h"
#include "temp_builtin.h"
#include "xpt2046.hpp"
#include "sd_card.hpp"
#include "usb_device.h"

// middlewares
#include "bridge_protocol.hpp"
#include "bridge_control.hpp"
#include "power.hpp"
#include "charger.hpp"
#include "gamepad.hpp"
#include "graphic_system.hpp"
#include "audio_system.hpp"
#include "led_control.hpp"
#include "temperature.hpp"
#include "touchscreen.hpp"
#include "vibration.hpp"
#include "imu.hpp"
#include "ir_link.hpp"

#include "gb_apu.hpp"
#include "gbcolors.h"
#include "peanut_gb.h"
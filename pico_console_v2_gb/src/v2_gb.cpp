// headers
#include "common.h"
#include "hw_test.hpp"
#include "v2_hw_def.h"

// hw lib init
ledStatus Led = ledStatus(PIN_LED_WL_1, PIN_LED_WL_2, PIN_LED_WL_3, PIN_LED_WL_4);
ili9488_40 Lcd = ili9488_40(PIN_DP_MOSI, PIN_DP_SCK, PIN_DP_CS, PIN_DP_DC, PIN_DP_RST, PIN_DP_BL);
xpt2046 Touch = xpt2046(HW_TOUCH_CH, PIN_TOUCH_MOSI, PIN_TOUCH_SCK, PIN_TOUCH_MISO, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
sdCard Sd = sdCard();
pio_uart_tx_t pio_tx;
pio_uart_rx_t pio_rx;
#if ENABLE_RFBRIDGE
pio_uart_tx_t pio_tx_rf;
pio_uart_rx_t pio_rx_rf;
#endif

// middleware lib init
bridgeProtocol Bridge = bridgeProtocol();
bridgeControl SouthBridge = bridgeControl(&Bridge);
#if ENABLE_RFBRIDGE
bridgeProtocol BridgeRf = bridgeProtocol();
bridgeControl RfBridge = bridgeControl(&BridgeRf);
#endif
power Power = power();
charger Charger = charger();
#if !ENABLE_HW_LED && ENABLE_SW_LED
ledControl LedCtrl = ledControl(&BridgeRf);
#else
ledControl LedCtrl = ledControl(&Led);
#endif
gamepad Gamepad = gamepad();
graphicSystem Graphic = graphicSystem(&Lcd);
audioSystem Audio = audioSystem();
temperature Temperature = temperature();
vibration Vibration = vibration();
touchscreen Touchscreen = touchscreen(&Touch);
imu Imu = imu();
irLink Ir = irLink();

void core1_entry();
void bridge_cmd_handler(const bridge_msg_t* msg);
#if ENABLE_RFBRIDGE
void bridge_cmd_handler_rf(const bridge_msg_t* msg);
#endif

time_ms_t bridge_timer;
time_ms_t led_timer;
time_ms_t gamepad_timer;
time_ms_t temperature_timer;
time_ms_t audio_timer;
time_ms_t vibration_timer;
time_ms_t touch_timer;
time_ms_t sd_timer;

inline int pio_uart_readable_wrapper(void) {
  return pio_uart_rx_readable(&pio_rx);
}
inline int pio_uart_read_wrapper(uint8_t* data, size_t buf_size) {
  return pio_uart_rx_read(&pio_rx, data, buf_size);
}
inline int pio_uart_writeable_wrapper(void) {
  return pio_uart_tx_writeable(&pio_tx);
}
inline int pio_uart_write_wrapper(const uint8_t* data, size_t data_size) {
  return pio_uart_tx_write(&pio_tx, data, data_size);
}

#if ENABLE_RFBRIDGE
inline int pio_uart_readable_wrapper_rf(void) {
  return pio_uart_rx_readable(&pio_rx_rf);
}
inline int pio_uart_read_wrapper_rf(uint8_t* data, size_t buf_size) {
  return pio_uart_rx_read(&pio_rx_rf, data, buf_size);
}
inline int pio_uart_writeable_wrapper_rf(void) {
  return pio_uart_tx_writeable(&pio_tx_rf);
}
inline int pio_uart_write_wrapper_rf(const uint8_t* data, size_t data_size) {
  return pio_uart_tx_write(&pio_tx_rf, data, data_size);
}
#endif
//////// function ////////

int main() { // uses core 0 to sub core
  // log init
  uartLog_init(HW_LOG_CH, PIN_LOG_TX, PIN_LOG_RX, HW_LOG_BAUD);
  uartLog_print("\n\npico console V2 booting...\n\n");

  // bridge init
  pio_uart_tx_init(&pio_tx, HW_BRIDGE_PIO, PIN_BRIDGE_TX, HW_BRIDGE_BAUD);
  pio_uart_rx_init(&pio_rx, HW_BRIDGE_PIO, PIN_BRIDGE_RX, HW_BRIDGE_BAUD);
  bridge_transport_t transport = {pio_uart_readable_wrapper, pio_uart_read_wrapper, pio_uart_writeable_wrapper, pio_uart_write_wrapper};
  Bridge.set_transport_handler(&transport);
  Bridge.set_cmd_handler(bridge_cmd_handler);
  SouthBridge.init();

#if ENABLE_RFBRIDGE // rf bridge init
  pio_uart_tx_init(&pio_tx_rf, HW_RF_BRIDGE_PIO, PIN_RF_BRIDGE_TX, HW_RF_BRIDGE_BAUD);
  pio_uart_rx_init(&pio_rx_rf, HW_RF_BRIDGE_PIO, PIN_RF_BRIDGE_RX, HW_RF_BRIDGE_BAUD);
  bridge_transport_t transport_rf = {pio_uart_readable_wrapper_rf, pio_uart_read_wrapper_rf, pio_uart_writeable_wrapper_rf, pio_uart_write_wrapper_rf};
  BridgeRf.set_transport_handler(&transport_rf);
  BridgeRf.set_cmd_handler(bridge_cmd_handler_rf);
  RfBridge.init();
#endif
  sleep_ms(100);

  LedCtrl.init();
  led_config_t led_config = {.mode = LED_BLINK_REPEAT, .brightness = 255, .update_interval_ms = 500};
  LedCtrl.set_config(LED_CTRL_BUILT_IN, led_config);
  LedCtrl.update();

  // initalizing hardwares
  Power.init();
  Charger.init();
  led_config = {.mode = LED_ON, .brightness = 255, .update_interval_ms = 20, .breathing_step = 10};
  LedCtrl.set_config(LED_CTRL_1, led_config);
  LedCtrl.set_config(LED_CTRL_2, led_config);
  LedCtrl.set_config(LED_CTRL_3, led_config);
  LedCtrl.set_config(LED_CTRL_4, led_config);
  LedCtrl.update();
  LOGI("LED ok\n");
#if ENABLE_PSRAM
  int32_t ret = psram_init(PIN_PSRAM_CS);
  if(ret < 0) {
    LOGE("PSRAM error : %d", ret);
    while(1);
  }
  LOGI("PSRAM ok\n");
#endif
  Graphic.begin();
  Graphic.fillScreen(LCD_BLACK);
  Graphic.set_bright(750);
  Graphic.setTextColor(LCD_WHITE, LCD_BLACK);
  Graphic.setTextSize(1);
  Graphic.set_font(G_FONT_5X8);
  LOGI("LCD ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("Touch init...");
  Touchscreen.init();
  Touchscreen.set_screen_size(480, 320);
  Touchscreen.set_calibration(200, 3800, 200, 3800); //placeholder
  Touchscreen.set_rotation(0);
  LOGI("Touch ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("Gamepad init...");
  Gamepad.init();
  Gamepad.set_enable(true, false);
  LOGI("Gamepad ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("TEMP init...");
  Temperature.init();
  LOGI("TEMP ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("LRA init...");
  Vibration.init();
  LOGI("Vibration ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("IMU init...");
  Imu.init();
  LOGI("IMU ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("IR init...");
  Ir.init();
  LOGI("IR ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("SD init...");
  Sd.init();
  LOGI("SD ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("USB init...");
  //board_init();
  usbDevice_init();
  LOGI("USB ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("               ");
  LOGI("all HWs ok!\n");
  LOGI("core freq = %ld hz\n", SYS_CLK_KHZ * 1000);
  // hardware initalized

  LOGI("go to main loop\n");
  multicore_launch_core1(core1_entry);
  // multicore_fifo_push_blocking(1);
  // boot sequence end

  while (true) {
    time_ms_t now_time = get_system_time_ms();

    Bridge.process_io();
    Bridge.dispatch_rx();
#if ENABLE_RFBRIDGE
    BridgeRf.process_io();
    BridgeRf.dispatch_rx();
#endif

    if(system_time_elapsed_ms(now_time, bridge_timer) > 1000) {
      bridge_timer = now_time;
      SouthBridge.update();
#if ENABLE_RFBRIDGE
      RfBridge.update();
#endif
    }
    if(system_time_elapsed_ms(now_time, led_timer) > 10) {
      led_timer = now_time;
      LedCtrl.update();
    }
    if(system_time_elapsed_ms(now_time, gamepad_timer) > 10) {
      gamepad_timer = now_time;
      Gamepad.update();
    }
    if(system_time_elapsed_ms(now_time, temperature_timer) > 1000) {
      temperature_timer = now_time;
      Temperature.update();
    }
    if(system_time_elapsed_ms(now_time, audio_timer) > 1) {
      audio_timer = now_time;
      Audio.update();
    }
    if(system_time_elapsed_ms(now_time, vibration_timer) > 10) {
      vibration_timer = now_time;
      Vibration.update();
    }
    if(system_time_elapsed_ms(now_time, touch_timer) > 10) {
      touch_timer = now_time;
      Touchscreen.update();
      //LOGT("x: %d, y: %d, z1: %d, z2: %d\n", Touchscreen.touch_data.x, Touchscreen.touch_data.y, Touchscreen.touch_data.z1, Touchscreen.touch_data.z2);
    }
    if(system_time_elapsed_ms(now_time, sd_timer) > 10) {
      sd_timer = now_time;
      Sd.update();
    }
    usbDevice_update();
  }

  return 0;
}

void core1_entry() { // uses core 1 to main core

  // multicore_fifo_pop_blocking(); // wait until boot process is done

  // boot animation
  Graphic.setTextSize(2);
  for(int i=0; i<160; i+=1) {
    Graphic.fillRect(150, i-1, (6*2*15), 1, LCD_BLACK);
    Graphic.setCursor(150,i);
    Graphic.print("PICO CONSOLE V2");
    sleep_ms(10);
  }

  Graphic.setCursor(480-(6*2*9),320-(8*2));
  Graphic.print("by Crem2y");
  Graphic.setTextSize(1);
  Graphic.setCursor(206,200);
  Graphic.print("press START");
  Graphic.setCursor(183,210);
  Graphic.print("or touch the screen");

  Graphic.setCursor(0,0);
  Graphic.print("press L/R to change bright");

  time_ms_t btn_time_ms = 0;
  time_ms_t display_time_ms = 0;
  bool display_text = false;
  bool display_bridge_status = false;
  while(true) {
    time_ms_t now_time = get_system_time_ms();
    if(Gamepad.is_btn_pressed(BTN_START)) break;
    if(Touchscreen.is_touched()) break;

    if(system_time_elapsed_ms(now_time, btn_time_ms) > 200) {
      btn_time_ms = now_time;

      uint16_t bright = Graphic.get_bright();
      if(Gamepad.is_btn_pressed(BTN_SL) && bright > 50) {
        Graphic.set_bright(bright - 50);
        Graphic.setCursor(0,8);
        Graphic.printf("bright : %d ", bright - 50);
      }
      if(Gamepad.is_btn_pressed(BTN_SR) && bright < 1000) {
        Graphic.set_bright(bright + 50);
        Graphic.setCursor(0,8);
        Graphic.printf("bright : %d ", bright + 50);
      }

      Graphic.setCursor(480-66,0);
      Graphic.printf("BAT:% 3.1f%%", Charger.get_bat_level());
    }

    if(system_time_elapsed_ms(now_time, display_time_ms) > 1000) {
      display_time_ms = now_time;
      if(display_text) {
        Graphic.fillRect(206,200,(6*11),8,LCD_BLACK);
      } else {
        Graphic.setCursor(206,200);
        Graphic.print("press START");
      }
      display_text = !display_text;
    }

    // to remove flickering
    if(!SouthBridge.connected) {
      if(!display_bridge_status) {
        Graphic.setCursor(162,240);
        Graphic.print("southbridge disconnected!");
        display_bridge_status = true;
      }
    } else {
      if(display_bridge_status) {
        Graphic.fillRect(162,240,(26*6),8,LCD_BLACK);
        display_bridge_status = false;
      }
    }
  }

  LedCtrl.set_mode(LED_CTRL_1, LED_DARKER);
  LedCtrl.set_mode(LED_CTRL_2, LED_DARKER);
  LedCtrl.set_mode(LED_CTRL_3, LED_DARKER);
  LedCtrl.set_mode(LED_CTRL_4, LED_DARKER);

  music_note_t boot_notes[2] = {
    {0, 6, 0, 32},   // C6
    {0, 7, 0, 32}    // C7
  };

  music_table_t boot_music = {
    .len = 2,
    .note_duration_ms = 100,
    .notes = boot_notes
  };

  Audio.set_master_config(127);
  for(int i=0; i<4; i++) {
    Audio.set_env(i, 25000, 1);
  }

  // if SELECT+START, quiet boot
  if(!Gamepad.is_btn_pressed(BTN_SELECT)) {
    Audio.play_music(&boot_music, false);
  }

  uint8_t cursor_x = 0;
  uint8_t cursor_x_old = 0;

main_menu_loop:
  Graphic.fillScreen(LCD_BLACK);
  Graphic.setTextColor(LCD_WHITE, LCD_BLACK);

  while (1) {
    Graphic.setTextSize(2);
    Graphic.setCursor(0,0);
    Graphic.print(" System info\n");
    Graphic.print(" Button test\n");
    Graphic.print(" Joystick test\n");
#if ENABLE_LED
    Graphic.print(" LED test\n");
#endif
#if ENABLE_PSRAM
    Graphic.print(" PSRAM test\n");
#endif
    Graphic.print(" LCD test\n");
    Graphic.print(" Touch test\n");
    Graphic.print(" Audio test\n");
    Graphic.print(" Vibration test\n");
    Graphic.print(" Battery & Power test\n");
    Graphic.print(" Temperature test\n");
    Graphic.print(" IR comm test\n");
    Graphic.print(" IMU test\n");
    Graphic.print(" SD card test\n");
    Graphic.print(" USB test\n");

    Graphic.setTextSize(1);
    Graphic.setCursor(0,320-(8*2));
    Graphic.print("press up/down to move cursor");
    Graphic.setCursor(0,320-(8*1));
    Graphic.print("press A or START to select");

    while(1) {
      sleep_ms(100);

      Graphic.setTextSize(1);
      Graphic.setCursor(480-66,0);
      Graphic.printf("BAT:% 3.1f%%", Charger.get_bat_level());

      Graphic.setTextSize(2);
      if(cursor_x_old != cursor_x) {
        Graphic.setCursor(0,cursor_x_old * 16);
        Graphic.print(" ");
      }
      Graphic.setCursor(0,cursor_x * 16);
      Graphic.print("-");

      cursor_x_old = cursor_x;

      if(Gamepad.is_btn_pressed(BTN_S1_UP) || Gamepad.is_btn_pressed(BTN_UP)) {
        if(cursor_x > 0) cursor_x--;
      }
      if(Gamepad.is_btn_pressed(BTN_S1_DOWN) || Gamepad.is_btn_pressed(BTN_DOWN)) {
        if(cursor_x < MAIN_USB_TEST) cursor_x++;
      }

      if(!Gamepad.is_btn_pressed(BTN_SELECT) && (Gamepad.is_btn_pressed(BTN_A) || Gamepad.is_btn_pressed(BTN_START))) {
        Graphic.fillScreen(LCD_BLACK);
        switch (cursor_x)
        {
        case MAIN_SYSTEM_INFO:
          menu_system_info();
          break;
        case MAIN_BTN_TEST:
          menu_btn_test();
          break;
        case MAIN_JOYSTICK_TEST:
          menu_joystick_test();
          break;
#if ENABLE_LED
        case MAIN_LED_TEST:
          menu_led_test();
          break;
#endif
#if ENABLE_PSRAM
        case MAIN_PSRAM_TEST:
          menu_psram_test();
          break;
#endif
        case MAIN_LCD_TEST:
          menu_lcd_test();
          break;
        case MAIN_TOUCH_TEST:
          menu_touch_test();
          break;
        case MAIN_AUDIO_TEST:
          menu_audio_test();
          break;
        case MAIN_VIBRATION_TEST:
          menu_vibration_test();
          break;
        case MAIN_BAT_TEST:
          menu_bat_test();
          break;
        case MAIN_TEMP_TEST:
          menu_temp_test();
          break;
        case MAIN_IR_TEST:
          menu_ir_test();
          break;
        case MAIN_IMU_TEST:
          menu_imu_test();
          break;
        case MAIN_SD_TEST:
          menu_sd_test();
          break;
        case MAIN_USB_TEST:
          menu_usb_test();
          break;
        default:
          cursor_x = 0;
          break;
        }
        goto main_menu_loop;
      }
    }
  }
}

//////// test menus ////////

void menu_system_info(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("System info");

  Graphic.setCursor(480-(6*2*9),320-(8*2));
  Graphic.print("by Crem2y");

  while(1) {
    sleep_ms(100);

    Graphic.setCursor(0,16*2);
    Graphic.setTextSize(1);
    Graphic.printf("SW version(MAIN): 0x%04X\n", SW_INFO_VERSION);
    Graphic.printf("build date      : %02d%02d%02d %02d%02d%02d\n\n", DATE_YY, DATE_MM, DATE_DD, TIME_HH, TIME_MM, TIME_SS);

    Graphic.printf("HW name         : %s\n", SouthBridge.info.hw_name);
    Graphic.printf("HW version      : 0x%04X\n", SouthBridge.info.hw_ver);
    Graphic.printf("HW support_flag : 0x%08X\n\n", SouthBridge.info.hw_support);

    Graphic.printf("SB Connected    : %s\n", SouthBridge.connected ? "Yes" : "No ");
    Graphic.printf("SW version (SB) : 0x%04X\n", SouthBridge.info.sw_ver);
    Graphic.printf("build date      : %06d %06d\n", SouthBridge.info.build_date, SouthBridge.info.build_time);
    Graphic.printf("SW support_flag : 0x%08X\n\n", SouthBridge.info.sw_support);
#if ENABLE_RFBRIDGE
    Graphic.printf("RF Connected    : %s\n", RfBridge.connected ? "Yes" : "No ");
    Graphic.printf("RF Module name  : %s\n", RfBridge.info.hw_name);
    Graphic.printf("SW version (RF) : 0x%04X\n", RfBridge.info.sw_ver);
    Graphic.printf("build date      : %06d %06d\n", RfBridge.info.build_date, RfBridge.info.build_time);
    Graphic.printf("SW support_flag : 0x%08X\n\n", RfBridge.info.sw_support);
#endif

    SouthBridge.read_hw_info();
    SouthBridge.read_hw_name();
    SouthBridge.read_sw_info();
#if ENABLE_RFBRIDGE
    RfBridge.read_hw_info();
    RfBridge.read_hw_name();
    RfBridge.read_sw_info();
#endif

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}

void menu_btn_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("Button test");
  
  Graphic.drawRect(20,64,280,156,LCD_WHITE);
  Graphic.drawRect(152,200,16,16,LCD_RED);

  while(1) {
    sleep_ms(10);
    
    if(Gamepad.is_btn_pressed(BTN_SL)) {
      if(Gamepad.get_btn_pressed_duration(BTN_SL) > 1000) {
        Graphic.fillRect(20,46,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(20,46,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(21,47,14,14,LCD_BLACK);
      Graphic.drawRect(20,46,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_SR)) {
      if(Gamepad.get_btn_pressed_duration(BTN_SR) > 1000) {
        Graphic.fillRect(284,46,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(284,46,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(285,47,14,14,LCD_BLACK);
      Graphic.drawRect(284,46,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_ZL)) {
      if(Gamepad.get_btn_pressed_duration(BTN_ZL) > 1000) {
        Graphic.fillRect(60,46,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(60,46,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(61,47,14,14,LCD_BLACK);
      Graphic.drawRect(60,46,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_ZR)) {
      if(Gamepad.get_btn_pressed_duration(BTN_ZR) > 1000) {
        Graphic.fillRect(244,46,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(244,46,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(245,47,14,14,LCD_BLACK);
      Graphic.drawRect(244,46,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_SELECT)) {
      if(Gamepad.get_btn_pressed_duration(BTN_SELECT) > 1000) {
        Graphic.fillRect(100,74,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(100,74,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(101,75,14,14,LCD_BLACK);
      Graphic.drawRect(100,74,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_START)) {
      if(Gamepad.get_btn_pressed_duration(BTN_START) > 1000) {
        Graphic.fillRect(205,74,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(205,74,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(206,75,14,14,LCD_BLACK);
      Graphic.drawRect(205,74,16,16,LCD_WHITE);
    }

    if(Gamepad.is_btn_pressed(BTN_S1_CENTER)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S1_CENTER) > 1000) {
        Graphic.fillRect(50,94,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(50,94,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(51,95,14,14,LCD_BLACK);
      Graphic.drawRect(50,94,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_S1_UP)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S1_UP) > 1000) {
        Graphic.fillRect(50,74,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(50,74,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(51,75,14,14,LCD_BLACK);
      Graphic.drawRect(50,74,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_S1_DOWN)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S1_DOWN) > 1000) {
        Graphic.fillRect(50,114,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(50,114,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(51,115,14,14,LCD_BLACK);
      Graphic.drawRect(50,114,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_S1_LEFT)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S1_LEFT) > 1000) {
        Graphic.fillRect(30,94,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(30,94,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(31,95,14,14,LCD_BLACK);
      Graphic.drawRect(30,94,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_S1_RIGHT)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S1_RIGHT) > 1000) {
        Graphic.fillRect(70,94,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(70,94,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(71,95,14,14,LCD_BLACK);
      Graphic.drawRect(70,94,16,16,LCD_WHITE);
    }

    if(Gamepad.is_btn_pressed(BTN_UP)) {
      if(Gamepad.get_btn_pressed_duration(BTN_UP) > 1000) {
        Graphic.fillRect(50,154,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(50,154,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(51,155,14,14,LCD_BLACK);
      Graphic.drawRect(50,154,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_DOWN)) {
      if(Gamepad.get_btn_pressed_duration(BTN_DOWN) > 1000) {
        Graphic.fillRect(50,194,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(50,194,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(51,195,14,14,LCD_BLACK);
      Graphic.drawRect(50,194,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_LEFT)) {
      if(Gamepad.get_btn_pressed_duration(BTN_LEFT) > 1000) {
        Graphic.fillRect(30,174,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(30,174,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(31,175,14,14,LCD_BLACK);
      Graphic.drawRect(30,174,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_RIGHT)) {
      if(Gamepad.get_btn_pressed_duration(BTN_RIGHT) > 1000) {
        Graphic.fillRect(70,174,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(70,174,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(71,175,14,14,LCD_BLACK);
      Graphic.drawRect(70,174,16,16,LCD_WHITE);
    }

    if(Gamepad.is_btn_pressed(BTN_S2_CENTER)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S2_CENTER) > 1000) {
        Graphic.fillRect(255,174,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(255,174,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(256,175,14,14,LCD_BLACK);
      Graphic.drawRect(255,174,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_X)) {
      if(Gamepad.get_btn_pressed_duration(BTN_X) > 1000) {
        Graphic.fillRect(255,74,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(255,74,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(256,75,14,14,LCD_BLACK);
      Graphic.drawRect(255,74,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_B)) {
      if(Gamepad.get_btn_pressed_duration(BTN_B) > 1000) {
        Graphic.fillRect(255,114,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(255,114,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(256,115,14,14,LCD_BLACK);
      Graphic.drawRect(255,114,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_Y)) {
      if(Gamepad.get_btn_pressed_duration(BTN_Y) > 1000) {
        Graphic.fillRect(235,94,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(235,94,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(236,95,14,14,LCD_BLACK);
      Graphic.drawRect(235,94,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_A)) {
      if(Gamepad.get_btn_pressed_duration(BTN_A) > 1000) {
        Graphic.fillRect(274,94,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(274,94,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(275,95,14,14,LCD_BLACK);
      Graphic.drawRect(274,94,16,16,LCD_WHITE);
    }
        if(Gamepad.is_btn_pressed(BTN_SUB1)) {
      if(Gamepad.get_btn_pressed_duration(BTN_SUB1) > 1000) {
        Graphic.fillRect(100,194,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(100,194,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(101,195,14,14,LCD_BLACK);
      Graphic.drawRect(100,194,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_SUB2)) {
      if(Gamepad.get_btn_pressed_duration(BTN_SUB2) > 1000) {
        Graphic.fillRect(205,194,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(205,194,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(206,195,14,14,LCD_BLACK);
      Graphic.drawRect(205,194,16,16,LCD_WHITE);
    }

    if(Gamepad.is_btn_pressed(BTN_S2_UP)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S2_UP) > 1000) {
        Graphic.fillRect(255,154,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(255,154,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(256,155,14,14,LCD_BLACK);
      Graphic.drawRect(255,154,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_S2_DOWN)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S2_DOWN) > 1000) {
        Graphic.fillRect(255,194,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(255,194,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(256,195,14,14,LCD_BLACK);
      Graphic.drawRect(255,194,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_S2_LEFT)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S2_LEFT) > 1000) {
        Graphic.fillRect(235,174,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(235,174,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(236,175,14,14,LCD_BLACK);
      Graphic.drawRect(235,174,16,16,LCD_WHITE);
    }
    if(Gamepad.is_btn_pressed(BTN_S2_RIGHT)) {
      if(Gamepad.get_btn_pressed_duration(BTN_S2_RIGHT) > 1000) {
        Graphic.fillRect(274,174,16,16,LCD_YELLOW);
      } else {
        Graphic.fillRect(274,174,16,16,LCD_WHITE);
      }
    } else {
      Graphic.fillRect(275,175,14,14,LCD_BLACK);
      Graphic.drawRect(274,174,16,16,LCD_WHITE);
    }


    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}

void menu_joystick_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("Joystick test");

  uint16_t prev_pos1_x, prev_pos1_y;
  uint16_t prev_pos2_x, prev_pos2_y;

  Graphic.drawRect(160-72, 160-72, 128+15, 128+15, LCD_WHITE);
  Graphic.drawRect(320-72, 160-72, 128+15, 128+15, LCD_WHITE);

  Gamepad.set_enable(true, true);

  while(1) {
    sleep_ms(10);

    int8_t joy1_x = Gamepad.get_joystick_data(JOY1_X);
    int8_t joy1_y = Gamepad.get_joystick_data(JOY1_Y);

    uint16_t pos1_x = 160 + (joy1_x >> 1);
    uint16_t pos1_y = 160 + (joy1_y >> 1);

    if(pos1_x != prev_pos1_x || pos1_y != prev_pos1_y) {
      Graphic.fillRect(prev_pos1_x-7,prev_pos1_y-7,14,14,LCD_BLACK);
      Graphic.drawCircle(159,159,71,LCD_WHITE);
      Graphic.drawLine(88,160,88+128+15,160,LCD_WHITE);
      Graphic.drawLine(160,88,160,88+128+15,LCD_WHITE);
      Graphic.fillRect(pos1_x-7,pos1_y-7,14,14,LCD_WHITE);
      prev_pos1_x = pos1_x;
      prev_pos1_y = pos1_y;

      Graphic.setCursor(0,16);
      Graphic.printf("L stick : % 4d, % 4d\n", joy1_x, joy1_y);
      Graphic.printf("L raw   :% 5d,% 5d\n", Gamepad.get_joystick_raw(JOY1_X), Gamepad.get_joystick_raw(JOY1_Y));
    }

    int8_t joy2_x = Gamepad.get_joystick_data(JOY2_X);
    int8_t joy2_y = Gamepad.get_joystick_data(JOY2_Y);

    uint16_t pos2_x = 320 + (joy2_x >> 1);
    uint16_t pos2_y = 160 + (joy2_y >> 1);

    if(pos2_x != prev_pos2_x || pos2_y != prev_pos2_y) {
      Graphic.fillRect(prev_pos2_x-7,prev_pos2_y-7,14,14,LCD_BLACK);
      Graphic.drawCircle(319,159,71,LCD_WHITE);
      Graphic.drawLine(248,160,248+128+15,160,LCD_WHITE);
      Graphic.drawLine(320,88,320,88+128+15,LCD_WHITE);
      Graphic.fillRect(pos2_x-7,pos2_y-7,14,14,LCD_WHITE);
      prev_pos2_x = pos2_x;
      prev_pos2_y = pos2_y;

      Graphic.setCursor(0,16*3);
      Graphic.printf("R stick : % 4d, % 4d\n", joy2_x, joy2_y);
      Graphic.printf("R raw   :% 5d,% 5d\n", Gamepad.get_joystick_raw(JOY2_X), Gamepad.get_joystick_raw(JOY2_Y));
    }

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      Gamepad.set_enable(true, false);
      return;
    }
  }
}

#if ENABLE_LED
void menu_led_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("LED test");

  Graphic.setCursor(0,16);
  Graphic.print("press START to change mode");

  int now_mode = LED_OFF;
  const char* led_mode_string[] = {
    "LED_OFF             ",
    "LED_ON              ",
    "LED_BLINK_ONCE      ",
    "LED_BLINK_REPEAT    ",
    "LED_BRIGHTER        ",
    "LED_DARKER          ",
    "LED_BREATHING_ONCE  ",
    "LED_BREATHING_REPEAT",
  };

  led_config_t led_config = {.mode = (enum led_ctrl_mode)now_mode, .brightness = 255, .update_interval_ms = 500, .breathing_step = 5};
  LedCtrl.set_config(LED_CTRL_1, led_config);
  LedCtrl.set_config(LED_CTRL_2, led_config);
  LedCtrl.set_config(LED_CTRL_3, led_config);
  LedCtrl.set_config(LED_CTRL_4, led_config);

  while(1) {
    sleep_ms(100);

    if(Gamepad.get_btn_released_duration(BTN_START) && Gamepad.get_btn_released_duration(BTN_START) < 100) {
      now_mode++;

      if(now_mode == LED_BLINK_ONCE || now_mode == LED_BLINK_REPEAT) {
        led_config.update_interval_ms = 500;
      } else if(now_mode == LED_BRIGHTER || now_mode == LED_DARKER ||
          now_mode == LED_BREATHING_ONCE || now_mode == LED_BREATHING_REPEAT) {
        led_config.update_interval_ms = 20;
      } else if(now_mode > LED_BREATHING_REPEAT) {
        now_mode = (int)LED_OFF;
      }
      led_config.mode = (enum led_ctrl_mode)now_mode;

      LedCtrl.set_config(LED_CTRL_1, led_config);
      LedCtrl.set_config(LED_CTRL_2, led_config);
      LedCtrl.set_config(LED_CTRL_3, led_config);
      LedCtrl.set_config(LED_CTRL_4, led_config);
    }
    Graphic.setCursor(0,16*2);
    Graphic.printf("LED mode : %s", led_mode_string[now_mode]);

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      LedCtrl.set_mode(LED_CTRL_1, LED_OFF);
      LedCtrl.set_mode(LED_CTRL_2, LED_OFF);
      LedCtrl.set_mode(LED_CTRL_3, LED_OFF);
      LedCtrl.set_mode(LED_CTRL_4, LED_OFF);
      return;
    }
  }
}
#endif

#if ENABLE_PSRAM
void menu_psram_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("PSRAM test");

  const size_t benchmark_size = 128 * 1024;
  const size_t benchmark_times = 8;

  Graphic.setCursor(0,16);
  Graphic.printf("PSRAM clock : %d MHz, size : %d MB\n", (APSXX04_MAX_HZ / 1000000), (psram_size / (1024*1024)));
  Graphic.printf("press A to benchmark SRAM & PSRAM\n");
  Graphic.printf("(%d kB x %d times)", (benchmark_size / 1024), benchmark_times);

  time_us_t sram_time_r = 0;
  time_us_t sram_time_w = 0;
  time_us_t psram_time_r = 0;
  time_us_t psram_time_w = 0;

  uint8_t* sram_buffer;
  uint8_t* psram_buffer = (uint8_t*)PSRAM_BASE;

  while(1) {
    sleep_ms(100);

    Graphic.setCursor(0,16*4);
    Graphic.printf("SRAM  R : %d us, W : %d us\n", sram_time_r, sram_time_w);
    Graphic.printf("PSRAM R : %d us, W : %d us", psram_time_r, psram_time_w);

    if(Gamepad.is_btn_pressed(BTN_A)) {
      uint8_t test_data = 0xA5;

      sram_buffer = (uint8_t*)malloc(benchmark_size);
      if(sram_buffer == NULL) continue;

      sram_time_w = get_system_time_us();
      for(volatile int i=0; i<benchmark_times; i++) {
        for(uint32_t j=0; j<benchmark_size; j++) {
            sram_buffer[j] = (uint8_t)(test_data + j);
        }
      }
      sram_time_w = system_time_elapsed_us(get_system_time_us(), sram_time_w);
      sram_time_r = get_system_time_us();
      for(volatile int i=0; i<benchmark_times; i++) {
        for(uint32_t j=0; j<benchmark_size; j++) {
          if(sram_buffer[j] != (uint8_t)(test_data + j)) {
            sram_time_r = 0;
            free(sram_buffer);
            continue;
          }
        }
      }
      sram_time_r = system_time_elapsed_us(get_system_time_us(), sram_time_r);
      free(sram_buffer);

      if(psram_size == 0) continue;
      psram_time_w = get_system_time_us();
      for(volatile int i=0; i<benchmark_times; i++) {
        for(uint32_t j=0; j<benchmark_size; j++) {
            psram_buffer[j] = (uint8_t)(test_data + j);
        }
      }
      psram_time_w = system_time_elapsed_us(get_system_time_us(), psram_time_w);
      psram_time_r = get_system_time_us();
      for(volatile int i=0; i<benchmark_times; i++) {
        for(uint32_t j=0; j<benchmark_size; j++) {
          if(psram_buffer[j] != (uint8_t)(test_data + j)) {
            psram_time_r = 0;
            continue;
          }
        }
      }
      psram_time_r = system_time_elapsed_us(get_system_time_us(), psram_time_r);
    }

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}
#endif

void menu_lcd_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("LCD test");

  int count = 0;
  uint16_t color = 0x0000;

  while(1) {
    sleep_ms(500);

    time_us_t start_time = get_system_time_us();

    switch(count) {
      case 0:
        Graphic.fillRect(0,16,480,(320-24),LCD_WHITE);
        break;
      case 1:
        Graphic.fillRect(0,16,480,(320-24),LCD_RED);
        break;
      case 2:
        Graphic.fillRect(0,16,480,(320-24),LCD_GREEN);
        break;
      case 3:
        Graphic.fillRect(0,16,480,(320-24),LCD_BLUE);
        break;
      case 4:
        Graphic.fillRect(0,16,480,(320-24),LCD_BLACK);
        for(int i=0; i<30; i++) {
          Graphic.setTextColor(color, LCD_BLACK);
          Graphic.setCursor(0+(rand() % (480-(6*2*8))), 16+(rand() % (320-24-(8*2))));
          Graphic.print("LCD test");
          color = rand();
        }
        break;
    }

    Graphic.setTextColor(LCD_WHITE, LCD_BLACK);
    Graphic.setCursor((480-(6*2*12)), 0);
    Graphic.printf("% 3.3f fps ", (1000000.0f / system_time_elapsed_us(get_system_time_us(), start_time)));

    count++;
    if(count > 4) count = 0;

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      Graphic.setTextColor(LCD_WHITE, LCD_BLACK);
      return;
    }
  }
}

void menu_touch_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("Touch test");

  time_ms_t touch_timer;

  Graphic.setCursor(0,16);
  Graphic.print("x:    0, y:    0");

  touch_point_t point;
  touch_point_t prev_point = {.x = -1, .y = -1};

  while(1) {
    sleep_ms(10);

    if(Touchscreen.is_touched()) {
      point = Touchscreen.get_touch_point();
      if(point.y >= 32 && point.y < (320-8)) {
        Graphic.setCursor(0,16);
        Graphic.printf("x: % 4d, y: % 4d", point.x, point.y);
        if(prev_point.x != -1 && prev_point.y != -1) {
          Graphic.drawLine(prev_point.x, prev_point.y, point.x, point.y, LCD_WHITE);
        } else {
          //Lcd.fillCircle(point.x, point.y, 1, LCD_WHITE);
          Graphic.drawPixel(point.x, point.y, LCD_WHITE);
        }
        prev_point = point;
      }
    } else {
      prev_point = {.x = -1, .y = -1};
    }

    if(Gamepad.is_btn_pressed(BTN_START)) {
      Graphic.fillRect(0,32,480,(320-40),LCD_BLACK);
    }

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}

music_note_t test_notes[4] = {
  {0, 4, 0, 32},   // C4
  {1, 4, 4, 32},   // E4
  {2, 4, 7, 32},   // G4
  {3, 5, 0, 32}    // C5
};

music_table_t test_music = {
  .len = 4,
  .note_duration_ms = 100,
  .notes = test_notes
};

void menu_audio_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("Audio test");

  Graphic.setCursor(0,16);
  Graphic.print("A : play music, B : play sound");

  for(int i=0; i<4; i++) {
    Audio.set_env(i, 25000, 1);
  }

  uint8_t master_vol = 127;

  // wave config
  uint8_t wave_vol = 32;
  float wave_freq = 100.0f;
  uint8_t wave_num = WAVE_SQUARE_12;
  // volume envelope config
  uint32_t env_tick = 25000;
  uint8_t env_step = 1;
  // pitch envelope config
  int32_t pit_tick = 25000;
  int8_t pit_target = 0;
  uint8_t pit_step = 1;

  Audio.set_wave(4, (wave_t)wave_num);
  Audio.set_env(4, env_tick, env_step);
  Audio.set_pitch_env(4, pit_tick, pit_target, pit_step);

  Audio.set_wave(0, WAVE_SQUARE_50);
  Audio.set_env(0, 25000, 1);

  while(1) {
    sleep_ms(10);

    Graphic.setCursor(0,16*3);
    Graphic.printf("master_vol: %d \n", master_vol);
    Graphic.printf("wave: %d, freq: %.3f, vol: %d \n", wave_num, wave_freq, wave_vol);
    Graphic.printf("env_tick: %d, env_step: %d  \n", env_tick, env_step);
    Graphic.printf("pit_tick: %d, pit_step: %d  \n", pit_tick, pit_step);
    Graphic.printf("pit_target: %d  \n", pit_target);

    int8_t joy1_x = Gamepad.get_joystick_data(JOY1_X);

    if(joy1_x < -10) {
      if(joy1_x < -64) {
        if(wave_freq > 5) wave_freq = pow(wave_freq, 0.995);
      } else {
        if(wave_freq > 5) wave_freq = pow(wave_freq, 0.999);
      }
    } else if(joy1_x > 10) {
      if(joy1_x > 64) {
        if(wave_freq < 10000) wave_freq = pow(wave_freq, 1.005);
      } else {
        if(wave_freq < 10000) wave_freq = pow(wave_freq, 1.001);
      }
    }
    if(Gamepad.is_btn_pressed(BTN_S1_UP)) {
      if(wave_vol != 255) wave_vol++;
      sleep_ms(100);
    }
    if(Gamepad.is_btn_pressed(BTN_S1_DOWN)) {
      if(wave_vol != 0) wave_vol--;
      sleep_ms(100);
    }

    if(Gamepad.is_btn_pressed(BTN_UP) || Gamepad.is_btn_pressed(BTN_DOWN) || Gamepad.is_btn_pressed(BTN_LEFT) || Gamepad.is_btn_pressed(BTN_RIGHT))
    {
      if(Gamepad.is_btn_pressed(BTN_UP)) {
        env_step++;
      }
      if(Gamepad.is_btn_pressed(BTN_DOWN)) {
        env_step--;
      }
      if(Gamepad.is_btn_pressed(BTN_LEFT)) {
        if(env_tick > 10) env_tick = pow(env_tick, 0.99);
      }
      if(Gamepad.is_btn_pressed(BTN_RIGHT)) {
        if(env_tick < 10000000) env_tick = pow(env_tick, 1.01);
      }
      Audio.set_env(4, env_tick, env_step);
      sleep_ms(100);
    }

    if(Gamepad.is_btn_pressed(BTN_S2_UP) || Gamepad.is_btn_pressed(BTN_S2_DOWN) || Gamepad.is_btn_pressed(BTN_S2_LEFT) || Gamepad.is_btn_pressed(BTN_S2_RIGHT)
      || Gamepad.is_btn_pressed(BTN_SUB1) || Gamepad.is_btn_pressed(BTN_SUB2))
    {
      if(Gamepad.is_btn_pressed(BTN_S2_UP)) {
        pit_step++;
      }
      if(Gamepad.is_btn_pressed(BTN_S2_DOWN)) {
        pit_step--;
      }
      if(Gamepad.is_btn_pressed(BTN_S2_LEFT)) {
        pit_tick -= 1000;
      }
      if(Gamepad.is_btn_pressed(BTN_S2_RIGHT)) {
        pit_tick += 1000;
      }
      if(Gamepad.is_btn_pressed(BTN_SUB1)) {
        pit_target--;
      }
      if(Gamepad.is_btn_pressed(BTN_SUB2)) {
        pit_target++;
      }
      Audio.set_pitch_env(4, pit_tick, pit_target, pit_step);
      sleep_ms(100);
    }

    if(Gamepad.is_btn_pressed(BTN_SL) || Gamepad.is_btn_pressed(BTN_SR)) {
      if(Gamepad.is_btn_pressed(BTN_SL)) {
        if(wave_num == WAVE_SQUARE_12) wave_num = WAVE_SINE;
        else wave_num--;
      }
      if(Gamepad.is_btn_pressed(BTN_SR)) {
        if(wave_num == WAVE_SINE) wave_num = WAVE_SQUARE_12;
        else wave_num++;
      }
      Audio.set_wave(4, (wave_t)wave_num);
      sleep_ms(100);
    }

    if(Gamepad.is_btn_pressed(BTN_SELECT) || Gamepad.is_btn_pressed(BTN_START)) {
      if(Gamepad.is_btn_pressed(BTN_START)) {
        if(master_vol < 240) master_vol += 10;
      }
      if(Gamepad.is_btn_pressed(BTN_SELECT)) {
        if(master_vol > 10) master_vol -= 10;
      }
      Audio.set_master_config(master_vol);
      sleep_ms(100);
    }

    if(Gamepad.is_btn_pressed(BTN_A)) {
      Audio.play_music(&test_music, false);
    }
    if(Gamepad.is_btn_pressed(BTN_X)) {
      Audio.stop_music();
    }
    if(Gamepad.is_btn_pressed(BTN_B)) {
      if(wave_num == WAVE_NOISE) {
        Audio.play_wave(4, wave_freq/1000, wave_vol);
      } else {
        Audio.play_wave(4, wave_freq, wave_vol);
      }
    }
    if(Gamepad.is_btn_pressed(BTN_Y)) {
      Audio.play_wave(4, wave_freq, 0);
    }

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}

void menu_vibration_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("Vibration test");

  uint16_t freq_l = 200, freq_r = 200;
  uint8_t power_l = 20, power_r = 20;

  while(1) {
    sleep_ms(100);

    Graphic.setCursor(0,16);
    Graphic.printf("LRA(L) : freq: % 5d, power: % 4d\n", freq_l, power_l);
    Graphic.printf("LRA(R) : freq: % 5d, power: % 4d\n", freq_r, power_r);

    if(Gamepad.is_btn_pressed(BTN_S1_UP)) {
      if(freq_l < 4000) freq_l += 10;
    }
    if(Gamepad.is_btn_pressed(BTN_S1_DOWN)) {
      if(freq_l > 10) freq_l -= 10;
    }
    if(Gamepad.is_btn_pressed(BTN_S1_LEFT)) {
      if(power_l > 0) power_l -= 1;
    }
    if(Gamepad.is_btn_pressed(BTN_S1_RIGHT)) {
      if(power_l < 100) power_l += 1;
    }

    if(Gamepad.is_btn_pressed(BTN_S2_UP)) {
      if(freq_r < 4000) freq_r += 10;
    }
    if(Gamepad.is_btn_pressed(BTN_S2_DOWN)) {
      if(freq_r > 10) freq_r -= 10;
    }
    if(Gamepad.is_btn_pressed(BTN_S2_LEFT)) {
      if(power_r > 0) power_r -= 1;
    }
    if(Gamepad.is_btn_pressed(BTN_S2_RIGHT)) {
      if(power_r < 100) power_r += 1;
    }

    if(Gamepad.is_btn_pressed(BTN_SL)) {
      Vibration.set_vibration(VIBRATION_L, freq_l, power_l);
    } else {
      Vibration.set_vibration(VIBRATION_L, 0, 0);
    }

    if(Gamepad.is_btn_pressed(BTN_SR)) {
      Vibration.set_vibration(VIBRATION_R, freq_r, power_r);
    } else {
      Vibration.set_vibration(VIBRATION_R, 0, 0);
    }

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      Vibration.set_vibration(VIBRATION_L, 0, 0);
      Vibration.set_vibration(VIBRATION_R, 0, 0);
      return;
    }
  }
}

void menu_bat_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("Battery & Power test");

  while(1) {
    sleep_ms(100);

    Graphic.setCursor(0,16);
    Graphic.printf("Vin : %01.3fV\n", Power.get_input_voltage());
    Graphic.printf("Battery : %s\n", Charger.get_battery_exist() ? "Yes" : "No ");
    Graphic.printf("Level : % 3.1f%% (%01.3fV)\n", Charger.get_bat_level(), Charger.get_bat_voltage());
    Graphic.printf("Charging : %s | Fault : 0x%02X", Charger.get_charging_status() ? "Yes" : "No ", Charger.get_fault_status());

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}

void menu_temp_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("Temperature test");

  while(1) {
    sleep_ms(100);

    Graphic.setCursor(0,16);
    Graphic.printf("TEMP_BUILTIN     : %.1f'C\n", Temperature.get_temp(TEMP_BUILTIN));
    Graphic.printf("TEMP_SOUTHBRIDGE : %.1f'C\n", Temperature.get_temp(TEMP_SOUTHBRIDGE));
    Graphic.printf("TEMP_NTC         : %.1f'C\n", Temperature.get_temp(TEMP_NTC));

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}

void menu_ir_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("IR comm test");

  Graphic.setCursor(0,16);
  Graphic.print("press START to change format");
  led_config_t led_config = {.mode = LED_OFF, .brightness = 255, .update_interval_ms = 100};
  LedCtrl.set_config(LED_CTRL_1, led_config); // RX indicator
  LedCtrl.set_config(LED_CTRL_4, led_config); // TX indicator
  Ir.set_enable_tx(true, IR_FORMAT_MANUAL);
  Ir.set_enable_rx(true, IR_FORMAT_MANUAL);
  
  while(1) {
    sleep_ms(100);

    Graphic.setCursor(0,16*2);
    Graphic.printf("format setting: %s", Ir.get_rx_format() == IR_FORMAT_NEC ? "NEC" : "RAW");
    if(Ir.is_data_ready() && Ir.get_rx_format() == Ir.get_rx_data_format()) {
      if(Ir.get_rx_data_format() == IR_FORMAT_MANUAL) {
        Graphic.setCursor(0,16*3);
        Graphic.printf("data: %d pulses\n", Ir.get_raw_data_pulses());
        Graphic.printf("%d, %d, %d, %d ... \n", ((uint16_t*)Ir.rx_data_buf)[0], ((uint16_t*)Ir.rx_data_buf)[1], ((uint16_t*)Ir.rx_data_buf)[2], ((uint16_t*)Ir.rx_data_buf)[3]);
        Graphic.print("press A to send data");
      } else if (Ir.get_rx_data_format() == IR_FORMAT_NEC) {
        Graphic.setCursor(0,16*3);
        Graphic.print("data:\n");
        Graphic.printf("%02X %02X %02X %02X\n", Ir.rx_data_buf[0], Ir.rx_data_buf[1], Ir.rx_data_buf[2], Ir.rx_data_buf[3]);
        Graphic.printf("press A to send data");
      }
    }

    if(Gamepad.is_btn_pressed(BTN_START)) {
      uint8_t temp = (uint8_t)Ir.get_rx_format();
      temp++;
      if(temp > IR_FORMAT_NEC) temp = IR_FORMAT_MANUAL;
      Ir.set_enable_rx(true, (enum ir_format)temp);
      Graphic.fillRect(0,16*3,480,(320-56),LCD_BLACK);
    }

    if(Gamepad.is_btn_pressed(BTN_A)) {
      if(Ir.is_data_ready()) {
        LedCtrl.set_mode(LED_CTRL_4, LED_BLINK_ONCE);
        Ir.send_bridge_tx_data(Ir.get_rx_data_format(), Ir.rx_data_buf, Ir.rx_data_len);
      }
      sleep_ms(500);
    } 

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      Ir.set_enable_tx(false, IR_FORMAT_MANUAL);
      Ir.set_enable_rx(false, IR_FORMAT_MANUAL);
      return;
    }
  }
}

void menu_imu_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("IMU test");

  Imu.set_enable(true, true);

  while(1) {
    sleep_ms(10);

    Graphic.setCursor(0,16);
    Graphic.printf("accel x: %2.4f \n", Imu.get_accel_x());
    Graphic.printf("accel y: %2.4f \n", Imu.get_accel_y());
    Graphic.printf("accel z: %2.4f \n", Imu.get_accel_z());

    Graphic.printf("gyro x: %4.2f \n", Imu.get_gyro_x());
    Graphic.printf("gyro y: %4.2f \n", Imu.get_gyro_y());
    Graphic.printf("gyro z: %4.2f \n", Imu.get_gyro_z());

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      Imu.set_enable(false, false);
      return;
    }
  }
}

static int global_printer_wrapper(const char* format, ...) {
  va_list args;
  va_start(args, format);
  
  int result = Graphic.vprintf(format, args); 
  
  va_end(args);
  return result;
}

void display_cat(const char *path) {
    FIL fil;
    FRESULT fr = f_open(&fil, path, FA_READ);
    if (FR_OK != fr) {
        Graphic.printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    while (f_gets(buf, sizeof buf, &fil)) {
        Graphic.printf("%s", buf);
    }
    if f_error(&fil)
        Graphic.printf("f_gets error\n");
    fr = f_close(&fil);
    if (FR_OK != fr) Graphic.printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
}

void ls_cursor(const char *dir, int cursor, char* cursor_path, uint8_t* cursor_type) {
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *p_dir;
    if (dir[0]) {
        p_dir = dir;
    } else {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr) {
            Graphic.printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj = {};      /* Directory object */
    FILINFO fno = {}; /* File information */
    assert(p_dir);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr) {
        Graphic.printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    int count = 0;
    uint8_t type = 0;
    *cursor_type = type;

    while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        /* Point pcAttrib to a string that describes the file. */
        if (fno.fattrib & AM_DIR) {
            pcAttrib = pcDirectory;
            type = 1;
        } else if (fno.fattrib & AM_RDO) {
            pcAttrib = pcReadOnlyFile;
            type = 2;
        } else {
            pcAttrib = pcWritableFile;
            type = 3;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        if(count == cursor) {
          strncpy(cursor_path, fno.fname, 512);
          Graphic.set_text_color(LCD_BLACK, LCD_WHITE);
          *cursor_type = type;
        } else {
          Graphic.set_text_color(LCD_WHITE, LCD_BLACK);
        }
        // Graphic.printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);
        Graphic.printf("%s [%s]\n", fno.fname, pcAttrib);

        fr = f_findnext(&dj, &fno); /* Search for next item */
        count++;
    }
    f_closedir(&dj);
}

void menu_sd_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("SD card test");

  Graphic.setCursor(0,16);
  Graphic.print("Loading...");

  enum sd_status status = SD_NO_CARD;
  enum sd_status prev_status = SD_CARD_ERR;

  bool displaying_info = false;
  bool need_display_update = true;

  char path[512] = "";
  char cursor_path[512] = "";
  uint8_t cursor = 0;
  uint8_t cursor_type = 0;
  bool file_reading = false;

  while(1) {
    sleep_ms(100);

    status = Sd.get_status();
    if(prev_status != status) {
      prev_status = status;
      Graphic.setCursor(0,16);
      Graphic.print("SD card : ");
      switch(status) {
        case SD_NO_CARD:
          Graphic.print("not inserted\n");
          strcpy(path, "");
          file_reading = false;
          cursor = 0;
          displaying_info = false;
          break;
        case SD_NOT_MOUNTED:
          Graphic.print("not mounted \n");
          break;
        case SD_MOUNTING:
          Graphic.print("mounting... \n");
          break;
        case SD_MOUNTED:
          Graphic.print("mounted     \n");
          break;
        case SD_CARD_ERR:
          Graphic.print("ERROR!!     \n");
          break;
        default:
          break;
      }
      need_display_update = true;
    }

    if(need_display_update) {
      need_display_update = false;
      Graphic.fillRect(0,16*2,480,(320-32),LCD_BLACK);
      if(status == SD_MOUNTED) {
        if(displaying_info) {
          Graphic.print("press START to hide info\n");
          Graphic.set_font(G_FONT_16);
          Sd.print_info(global_printer_wrapper);
          Graphic.set_font(G_FONT_5X8);
          strcpy(path, "");
          cursor = 0;
          cursor_type = 0;
        } else {
          Graphic.print("press START to display info\n\n");
          Graphic.set_font(G_FONT_16);
          FRESULT fr = f_getcwd(path, 512);
          if (FR_OK == fr) {
            if(file_reading) {
              Graphic.printf("data of '%s'\n", cursor_path);
              display_cat(cursor_path);
            } else {
              Graphic.printf("list of '%s'\n", path);
              ls_cursor(path, cursor, cursor_path, &cursor_type);
            }
          }
          Graphic.set_font(G_FONT_5X8);
          Graphic.set_text_color(LCD_WHITE, LCD_BLACK);
        }
      }
    }

    if(Gamepad.is_btn_pressed(BTN_START)) {
      displaying_info = !displaying_info;
      need_display_update = true;
    }
    if(Gamepad.is_btn_pressed(BTN_A)) {
      if(cursor_type) {
        if(cursor_type == 1) { // directory
          f_chdir(cursor_path);
          cursor = 0;
        } else { // file
          file_reading = true;
        }
        need_display_update = true;
      }
    }
    if(Gamepad.is_btn_pressed(BTN_B)) {
      if(file_reading) {
        file_reading = false;
      } else {
        f_chdir("..");
        cursor = 0;
      }
      need_display_update = true;
    }

    if(Gamepad.is_btn_pressed(BTN_S1_UP)) {
      if(cursor > 0) cursor--;
      need_display_update = true;
    }
    if(Gamepad.is_btn_pressed(BTN_S1_DOWN)) {
      if(cursor < 128) cursor++;
      need_display_update = true;
    }


    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      return;
    }
  }
}

typedef struct _btn_map_t {
  enum btn_code btn;
  uint32_t key;
} btn_map_t;

btn_map_t btn_keyboard[22] = { // map button code to keyboard key
  {BTN_UP, HID_KEY_ARROW_UP},
  {BTN_DOWN, HID_KEY_ARROW_DOWN},
  {BTN_LEFT, HID_KEY_ARROW_LEFT},
  {BTN_RIGHT, HID_KEY_ARROW_RIGHT},
  {BTN_A, HID_KEY_Z},
  {BTN_B, HID_KEY_X},
  {BTN_X, HID_KEY_C},
  {BTN_Y, HID_KEY_V},
  {BTN_SL, HID_KEY_Q},
  {BTN_SR, HID_KEY_E},
  {BTN_ZL, HID_KEY_R},
  {BTN_ZR, HID_KEY_T},
  {BTN_START, HID_KEY_F},
  {BTN_SELECT, HID_KEY_G},
  {BTN_SUB1, HID_KEY_CONTROL_LEFT},
  {BTN_SUB2, HID_KEY_ALT_LEFT},
  {BTN_S1_CENTER, HID_KEY_SPACE},
  {BTN_S2_CENTER, HID_KEY_SHIFT_LEFT},

  {BTN_S1_UP, HID_KEY_W},
  {BTN_S1_DOWN, HID_KEY_S},
  {BTN_S1_LEFT, HID_KEY_A},
  {BTN_S1_RIGHT, HID_KEY_D},
};

btn_map_t btn_gamepad[18] = { // map button code to gamepad button
  {BTN_B, 0x00000001},
  {BTN_A, 0x00000002},
  {BTN_Y, 0x00000004},
  {BTN_X, 0x00000008},
  {BTN_SL, 0x00000010},
  {BTN_SR, 0x00000020},
  {BTN_ZL, 0x00000040},
  {BTN_ZR, 0x00000080},
  {BTN_SELECT, 0x00000100},
  {BTN_START, 0x00000200},
  {BTN_S1_CENTER, 0x00000400},
  {BTN_S2_CENTER, 0x00000800},
  {BTN_UP, 0x00001000},
  {BTN_DOWN, 0x00002000},
  {BTN_LEFT, 0x00004000},
  {BTN_RIGHT, 0x00008000},
  {BTN_SUB2, 0x00010000},
  {BTN_SUB1, 0x00020000},
};

void menu_usb_test(void) {
  Graphic.setTextSize(1);
  Graphic.setCursor(0,320-8);
  Graphic.print("press SELECT & START to exit menu");
  Graphic.setTextSize(2);
  Graphic.setCursor(0,0);
  Graphic.print("USB test");

  uint8_t hid_mode = 0; // 0: keyboard & mouse, 1: gamepad
  uint8_t key_codes[6] = {0,}; // max 6 key presses

  touch_point_t touch_data = {.x = 0, .y = 0};
  touch_point_t prev_touch_data = {.x = 0, .y = 0};
  usb_device_mouse_t mouse_data = {0,};

  usb_device_gamepad_t gamepad_data = {0,};

  Graphic.setCursor(0,16);
  Graphic.print("HID mode: Keyboard & Mouse\n");

  while(1) {
    sleep_ms(10);

    if(Gamepad.is_btn_pressed(BTN_UP) && Gamepad.is_btn_pressed(BTN_DOWN)) {
      // reset hid data
      if(hid_mode == HID_MODE_KEYBOARD_MOUSE) {
        usbDevice_update_keyboard(false, NULL);
        memset(&mouse_data, 0, sizeof(mouse_data));
        usbDevice_update_mouse(&mouse_data);
      } else if(hid_mode == HID_MODE_GAMEPAD) {
        memset(&gamepad_data, 0, sizeof(gamepad_data));
        usbDevice_update_gamepad(&gamepad_data);
      }

      hid_mode++;
      if(hid_mode > HID_MODE_GAMEPAD) hid_mode = HID_MODE_KEYBOARD_MOUSE;

      if(hid_mode == HID_MODE_KEYBOARD_MOUSE) {
        Graphic.setCursor(0,16);
        Graphic.print("HID mode: Keyboard & Mouse\n");
      } else if(hid_mode == HID_MODE_GAMEPAD) {
        Graphic.setCursor(0,16);
        Graphic.print("HID mode: Gamepad         \n");
      }
      usbDevice_set_hid_mode((enum hid_mode_t)hid_mode);
      sleep_ms(500);
    }

    if(hid_mode == HID_MODE_KEYBOARD_MOUSE) { // keyboard & mouse
      bool changed = false;
      for (size_t i = 0; i < 6; i++)
      {
        key_codes[i] = 0;
      }

      uint8_t index = 0;
      for(int i=0; i<22; i++) {
        if(Gamepad.is_btn_pressed(btn_keyboard[i].btn)) {
          key_codes[index] = btn_keyboard[i].key; // set keycode
          changed = true;
          index++;
          if (index >= 6) // max is 6 key presses
          {
            break;
          }
        }
      }
      usbDevice_update_keyboard(changed, key_codes);

      mouse_data.buttons = Gamepad.is_btn_pressed(BTN_A) ? 0x01 : 0; 
      touch_data = Touchscreen.get_touch_point();
      if(touch_data.x > 0 && touch_data.y > 0) {
        mouse_data.x = (touch_data.x - prev_touch_data.x) * 5.0f;
        mouse_data.y = (touch_data.y - prev_touch_data.y) * 5.0f;
      } else {
        mouse_data.x = 0;
        mouse_data.y = 0;
      }
      prev_touch_data = touch_data;
      usbDevice_update_mouse(&mouse_data);
    } else if(hid_mode == HID_MODE_GAMEPAD) { // gamepad
      gamepad_data.buttons = 0;
      for(int i=0; i<18; i++) {
        if(Gamepad.is_btn_pressed(btn_gamepad[i].btn)) {
          gamepad_data.buttons |= btn_gamepad[i].key; // set button
        }
      }
      gamepad_data.hat = 0;
      gamepad_data.s1_x = Gamepad.get_joystick_data(JOY1_X);
      gamepad_data.s1_y = Gamepad.get_joystick_data(JOY1_Y);
      gamepad_data.s2_x = Gamepad.get_joystick_data(JOY2_X);
      gamepad_data.s2_y = Gamepad.get_joystick_data(JOY2_Y);

      usbDevice_update_gamepad(&gamepad_data);
    }

    if(Gamepad.is_btn_pressed(BTN_SELECT) && Gamepad.is_btn_pressed(BTN_START)) {
      sleep_ms(10);
      usbDevice_update_keyboard(false, NULL);
      memset(&mouse_data, 0, sizeof(mouse_data));
      usbDevice_update_mouse(&mouse_data);
      memset(&gamepad_data, 0, sizeof(gamepad_data));
      usbDevice_update_gamepad(&gamepad_data);
      return;
    }
  }
}

void bridge_cmd_handler(const bridge_msg_t* msg) {
  enum bridge_cmd command = (enum bridge_cmd)msg->cmd;
  SouthBridge.update_last_comm_time();

  switch (command)
  {
  case CMD_HW_INFO_RES:
    SouthBridge.recv_bridge_hw_info_res(msg->payload, msg->payload_size);
    break;
  case CMD_HW_NAME_RES:
    SouthBridge.recv_bridge_hw_name_res(msg->payload, msg->payload_size);
    break;
  case CMD_SW_INFO_RES:
    SouthBridge.recv_bridge_sw_info_res(msg->payload, msg->payload_size);
    break;
  case CMD_TEMPERATURE_DATA:
    Temperature.recv_bridge_data(msg->payload, msg->payload_size);
    break;
  case CMD_POWER_STATUS:
    Power.recv_bridge_power_status(msg->payload, msg->payload_size);
    break;
  case CMD_BATTERY_STATUS:
    Charger.recv_bridge_bat_status(msg->payload, msg->payload_size);
    break;
  case CMD_GAMEPAD_DATA:
    Gamepad.recv_bridge_data(msg->payload, msg->payload_size);
    break;
  case CMD_GAMEPAD_RAW_DATA:
    Gamepad.recv_bridge_raw_data(msg->payload, msg->payload_size);
    break;
  case CMD_IR_RX_DATA:
    Ir.recv_bridge_rx_data(msg->payload, msg->payload_size);
    break;
  case CMD_IMU_ACCEL_DATA:
    Imu.recv_bridge_accel_data(msg->payload, msg->payload_size);
    break;
  case CMD_IMU_GYRO_DATA:
    Imu.recv_bridge_gyro_data(msg->payload, msg->payload_size);
    break;
  default:
    break;
  }
}

#if ENABLE_RFBRIDGE
void bridge_cmd_handler_rf(const bridge_msg_t* msg) {
  enum bridge_cmd command = (enum bridge_cmd)msg->cmd;
  RfBridge.update_last_comm_time();

  switch (command)
  {
  case CMD_HW_INFO_RES:
    RfBridge.recv_bridge_hw_info_res(msg->payload, msg->payload_size);
    break;
  case CMD_HW_NAME_RES:
    RfBridge.recv_bridge_hw_name_res(msg->payload, msg->payload_size);
    break;
  case CMD_SW_INFO_RES:
    RfBridge.recv_bridge_sw_info_res(msg->payload, msg->payload_size);
    break;
  default:
    break;
  }
}
#endif
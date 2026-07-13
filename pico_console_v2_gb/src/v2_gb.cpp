// headers
#include "common.h"
#include "v2_gb.hpp"
#include "v2_hw_def.h"

#define SCALING_2X 1

// hw lib init
ledStatus Led = ledStatus(PIN_LED_WL_1, PIN_LED_WL_2, PIN_LED_WL_3, PIN_LED_WL_4);
ili9488_40 Lcd = ili9488_40(PIN_DP_MOSI, PIN_DP_SCK, PIN_DP_CS, PIN_DP_DC, PIN_DP_RST, PIN_DP_BL);
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

/* Multicore command structure. */
union core_cmd {
    struct {
	/* Does nothing. */
#define CORE_CMD_NOP		0
	/* Set line "data" on the LCD. Pixel data is in pixels_buffer. */
#define CORE_CMD_LCD_LINE	1
	/* Control idle mode on the LCD. Limits colours to 2 bits. */
#define CORE_CMD_IDLE_SET	2
	/* Set a specific pixel. For debugging. */
#define CORE_CMD_SET_PIXEL	3
	uint8_t cmd;
	uint8_t unused1;
	uint8_t unused2;
	uint8_t data;
    };
    uint32_t full;
};

static uint8_t pixels_buffer[LCD_WIDTH];

extern const uint8_t rom_data[];
const uint8_t* rom = rom_data;
static unsigned char rom_bank0[65536];

static uint8_t ram[32768];
static int lcd_line_busy = 0;
static palette_t palette;	// Colour palette
static uint8_t manual_palette_selected=0;

/**
 * Returns a byte from the ROM file at the given address.
 */
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
  (void) gb;
  if(addr < sizeof(rom_bank0))
    return rom_bank0[addr];

  return rom[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address.
 */
uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
  (void) gb;
  return ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address.
 */
void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
		       const uint8_t val)
{
  ram[addr] = val;
}

/**
 * Ignore all errors.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr)
{
#if 1
  const char* gb_err_str[4] = {
      "UNKNOWN",
      "INVALID OPCODE",
      "INVALID READ",
      "INVALID WRITE"
    };
  printf("Error %d occurred: %s at %04X\n.\n", gb_err, gb_err_str[gb_err], addr);
//	abort();
#endif
}

void core0_lcd_draw_line(const uint_fast8_t line)
{
#if SCALING_2X
  static uint16_t fb[LCD_WIDTH*2];

  for(unsigned int x = 0; x < LCD_WIDTH; x++)
  {
    fb[x*2] = palette[(pixels_buffer[x] & LCD_PALETTE_ALL) >> 4]
        [pixels_buffer[x] & 3];
    fb[x*2+1] = fb[x*2];
  }

  Graphic.draw_picture(80, 16+(line*2),   LCD_WIDTH*2, 1, fb);
  Graphic.draw_picture(80, 16+(line*2)+1, LCD_WIDTH*2, 1, fb);
  __atomic_store_n(&lcd_line_busy, 0, __ATOMIC_SEQ_CST);
#else
  static uint16_t fb[LCD_WIDTH];

  for(unsigned int x = 0; x < LCD_WIDTH; x++)
  {
    fb[x] = palette[(pixels_buffer[x] & LCD_PALETTE_ALL) >> 4]
        [pixels_buffer[x] & 3];
  }

  Graphic.draw_picture(160, 88+(line), LCD_WIDTH, 1, fb);
  __atomic_store_n(&lcd_line_busy, 0, __ATOMIC_SEQ_CST);
#endif
}

void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[LCD_WIDTH],
		   const uint_fast8_t line)
{
  union core_cmd cmd;

  /* Wait until previous line is sent. */
  while(__atomic_load_n(&lcd_line_busy, __ATOMIC_SEQ_CST))
    tight_loop_contents();

  memcpy(pixels_buffer, pixels, LCD_WIDTH);

  /* Populate command. */
  cmd.cmd = CORE_CMD_LCD_LINE;
  cmd.data = line;

  __atomic_store_n(&lcd_line_busy, 1, __ATOMIC_SEQ_CST);
  multicore_fifo_push_blocking(cmd.full);
}

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
  Graphic.print("Gamepad init...");
  Gamepad.init();
  Gamepad.set_enable(true, false);
  LOGI("Gamepad ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("TEMP init...");
  Temperature.init();
  LOGI("TEMP ok\n");
  Graphic.setCursor(0,0);
  Graphic.print("SD init...");
  Sd.init();
  LOGI("SD ok\n");
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
    if(system_time_elapsed_ms(now_time, sd_timer) > 10) {
      sd_timer = now_time;
      Sd.update();
    }

    union core_cmd cmd;
    if(multicore_fifo_rvalid()) {
      cmd.full = multicore_fifo_pop_blocking();
      switch(cmd.cmd)
      {
      case CORE_CMD_LCD_LINE:
        core0_lcd_draw_line(cmd.data);
        break;

      case CORE_CMD_IDLE_SET:
  //			mk_ili9225_display_control(true, cmd.data);
        break;

      case CORE_CMD_NOP:
      default:
        break;
      }
    }
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

  static struct gb_s gb;
	enum gb_init_error_e ret;

  while (1) {
    Graphic.fillScreen(LCD_BLACK);
#if SCALING_2X
    Graphic.draw_rect(80, 16, LCD_WIDTH*2, LCD_HEIGHT*2, LCD_WHITE);
#else
    Graphic.draw_rect(160, 88, LCD_WIDTH, LCD_HEIGHT, LCD_WHITE);
#endif
    Graphic.setTextColor(LCD_WHITE, LCD_BLACK);
//    rom_file_selector();

    /* Initialise GB context. */
    memcpy(rom_bank0, rom, sizeof(rom_bank0));
    ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read,
            &gb_cart_ram_write, &gb_error, NULL);
    if(ret != GB_INIT_NO_ERROR)
    {
      LOGE("Error: %d\n", ret);
      LOGI("Emulation Ended");
      continue;
    }

    /* Automatically assign a colour palette to the game */
    char rom_title[16];
    auto_assign_palette(palette, gb_colour_hash(&gb),gb_get_rom_name(&gb,rom_title));

    gb_init_lcd(&gb, &lcd_draw_line);
    uint_fast32_t frames = 0;

    uint64_t start_time = time_us_64();
    while(1)
    {
      int input;

      gb.gb_frame = 0;

      do {
        __gb_step_cpu(&gb);
        tight_loop_contents();
      } while(gb.gb_frame == 0);

      frames++;

      gb.direct.joypad_bits.up      = !Gamepad.is_btn_pressed(BTN_S1_UP);
      gb.direct.joypad_bits.down    = !Gamepad.is_btn_pressed(BTN_S1_DOWN);
      gb.direct.joypad_bits.left    = !Gamepad.is_btn_pressed(BTN_S1_LEFT);
      gb.direct.joypad_bits.right   = !Gamepad.is_btn_pressed(BTN_S1_RIGHT);
      gb.direct.joypad_bits.a       = !Gamepad.is_btn_pressed(BTN_A);
      gb.direct.joypad_bits.b       = !Gamepad.is_btn_pressed(BTN_B);
      gb.direct.joypad_bits.select  = !Gamepad.is_btn_pressed(BTN_SELECT);
      gb.direct.joypad_bits.start   = !Gamepad.is_btn_pressed(BTN_START);
    }

    LOGI("Emulation Ended");
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
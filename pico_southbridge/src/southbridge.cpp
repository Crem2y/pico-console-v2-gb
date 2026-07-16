// headers
#include "southbridge.hpp"
#include "southbridge_hw_def.h"

// hw lib init
btn_matrix BtnMatrix = btn_matrix(PIN_BTN_H1, PIN_BTN_H2, PIN_BTN_H3, PIN_BTN_H4, PIN_BTN_V1, PIN_BTN_V2, PIN_BTN_V3, PIN_BTN_V4);
joystick Joy1 = joystick(PIN_JOY1_X, PIN_JOY1_Y, false, false, PIN_JOY1_BTN);
joystick Joy2 = joystick(PIN_JOY2_X, PIN_JOY2_Y, true,  true,  PIN_JOY2_BTN);
vibrationLRA Lra = vibrationLRA(PIN_LRA_L, PIN_LRA_R);
bq25619 Bq25619 = bq25619(HW_I2C_CH, PIN_I2C_SDA, PIN_I2C_SCL, PIN_BAT_INT);
mpu6050 Mpu = mpu6050(HW_I2C_CH, PIN_I2C_SDA, PIN_I2C_SCL, PIN_IMU_INT);
tempNTC TempSensor = tempNTC(PIN_NTC);
AdcVSense VSenseVBAT = AdcVSense(PIN_VBAT, HW_VBAT_RATIO);
AdcVSense VSenseVIN = AdcVSense(PIN_VIN, HW_VIN_RATIO);
ir_pulse_capture_t ir_rx;
ir_tx_t ir_tx;

// middleware lib init
bridgeProtocol Bridge = bridgeProtocol();
bridgeControl MainBridge = bridgeControl();
power Power = power(&VSenseVIN);
charger Charger = charger(&VSenseVBAT, &Bq25619);
gamepad Gamepad = gamepad(&BtnMatrix, &Joy1, &Joy2);
audioSystem Audio = audioSystem();
temperature Temperature = temperature(&TempSensor);
vibration Vibration = vibration(&Lra);
imu Imu = imu(&Mpu);
irLink Ir = irLink(&ir_rx, &ir_tx);

void core1_entry(void);
void bridge_do_cmd(const bridge_msg_t* msg);

time_ms_t bridge_timer;
time_ms_t gamepad_timer;
time_ms_t temperature_timer;
time_ms_t imu_timer;
time_ms_t battery_timer;
time_ms_t ir_timer;

//////// function ////////

int main() {
//  uartLog_init(HW_LOG_CH, PIN_LOG_TX, PIN_LOG_RX, HW_LOG_BAUD);
  stdio_init_all();
  uart_bridge_init(HW_BRIDGE_CH, PIN_BRIDGE_TX, PIN_BRIDGE_RX, HW_BRIDGE_BAUD);
  bridge_transport_t transport = {uart_bridge_readable, uart_bridge_read, uart_bridge_writable, uart_bridge_write};
  Bridge.set_transport_handler(&transport);
  Bridge.set_cmd_handler(bridge_do_cmd);
  MainBridge.init();
  MainBridge.my_info.hw_ver = HW_INFO_VERSION;
  MainBridge.my_info.hw_support = HW_INFO_SUPPORT;
  strncpy(MainBridge.my_info.hw_name, HW_NAME, PAYLOAD_MAX_SIZE);
  MainBridge.my_info.build_date = DATE_YY*10000 + DATE_MM*100 + DATE_DD;
  MainBridge.my_info.build_time = TIME_HH*10000 + TIME_MM*100 + TIME_SS;
  MainBridge.my_info.sw_ver = SW_INFO_VERSION;
  MainBridge.my_info.sw_support = SW_INFO_SUPPORT;

  audio_init(PIN_I2S_DATA, PIN_I2S_SCK, PIN_DAC_MUTE);
  Audio.init();
  Audio.set_enable(true);
  Gamepad.init();
  Temperature.init();
  TempSensor.init();
  Vibration.init();
  Vibration.enable(true);
  Power.init();
  Charger.init();
  Imu.init();
  ir_pulse_capture_init(&ir_rx, pio1, PIN_IR_RX); //placeholder for pio
  ir_tx_init(&ir_tx, pio2, PIN_IR_TX); //placeholder for pio
  Ir.init();

  sleep_ms(100);

  multicore_launch_core1(core1_entry);
  uart_bridge_enable_irq();

  // boot sequence end
  Bq25619.read_reg(BQ25619_REG_CHARGE_CURRENT_LIMIT);
  printf("before ichg : %dmA\n", Bq25619.get_charge_current());

  Bq25619.set_charge_current(500);

  Bq25619.read_reg(BQ25619_REG_CHARGE_CURRENT_LIMIT);
  printf("after ichg : %dmA\n", Bq25619.get_charge_current());

  while (true) {
    time_ms_t now_time = get_system_time_ms();

    Bridge.process_io();
    Bridge.dispatch_rx();

    if(system_time_elapsed_ms(now_time, bridge_timer) > 1000) {
      bridge_timer = now_time;
      MainBridge.update();
    }
    if(system_time_elapsed_ms(now_time, gamepad_timer) > 10) {
      gamepad_timer = now_time;
      Gamepad.update();
    }
    if(system_time_elapsed_ms(now_time, temperature_timer) > 1000) {
      temperature_timer = now_time;
      Temperature.update();
    }
    if(system_time_elapsed_ms(now_time, imu_timer) > 10) {
      imu_timer = now_time;
      Imu.update();
    }
    if(system_time_elapsed_ms(now_time, battery_timer) > 1000) {
      battery_timer = now_time;
      Charger.update();
      Power.update();
    }
    if(system_time_elapsed_ms(now_time, ir_timer) > 1) {
      ir_timer = now_time;
      Ir.update();
    }
  }

  return 0;
}

void core1_entry(void) {

//  multicore_fifo_pop_blocking(); // wait until boot process is done

  while (true) {
    //sleep_ms(10);
    audio_loop();
  }
}

void bridge_do_cmd(const bridge_msg_t* msg) {
  enum bridge_cmd command = (enum bridge_cmd)msg->cmd;
  switch (command)
  {
  case CMD_HW_INFO_REQ:
    MainBridge.send_bridge_hw_info_res();
    break;
  case CMD_HW_NAME_REQ:
    MainBridge.send_bridge_hw_name_res();
    break;
  case CMD_SW_INFO_REQ:
    MainBridge.send_bridge_sw_info_res();
    break;
  case CMD_POWER_CONTROL:
    break;
  case CMD_BATTERY_CONTROL:
    Charger.recv_bridge_bat_control(msg->payload, msg->payload_size);
    break;
  case CMD_GAMEPAD_ENABLE:
    Gamepad.recv_bridge_enable(msg->payload, msg->payload_size);
    break;
  case CMD_GAMEPAD_CALI_DATA:
    Gamepad.recv_bridge_cali_data(msg->payload, msg->payload_size);
    break;
  case CMD_IR_RX_ENABLE:
    Ir.recv_bridge_enable_rx(msg->payload, msg->payload_size);
    break;
  case CMD_IR_RX_DISABLE:
    Ir.enable_rx(false);
    break;
  case CMD_IR_TX_ENABLE:
    Ir.recv_bridge_enable_tx(msg->payload, msg->payload_size);
    break;
  case CMD_IR_TX_DATA:
    Ir.recv_bridge_tx_data(msg->payload, msg->payload_size);
    break;
  case CMD_IR_TX_DISABLE:
    Ir.enable_tx(false);
    break;
  case CMD_IMU_ENABLE:
    Imu.recv_bridge_enable(msg->payload, msg->payload_size);
    break;
  case CMD_IMU_DISABLE:
    Imu.enable_accel_data(false);
    Imu.enable_gyro_data(false);
    break;
  case CMD_AUDIO_ENABLE:
    Audio.set_enable(true);
    break;
  case CMD_AUDIO_NOTE_ON:
    Audio.recv_bridge_note_on(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_SET_FREQ:
    Audio.recv_bridge_set_freq(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_SET_VOL:
    Audio.recv_bridge_set_vol(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_SET_WAVE:
    Audio.recv_bridge_set_wave(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_SET_MIX:
    Audio.recv_bridge_set_mix(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_SET_VOL_ENV:
    Audio.recv_bridge_set_vol_env(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_SET_PIT_ENV:
    Audio.recv_bridge_set_pitch_env(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_WAVE_DATA32:
    Audio.recv_bridge_wave_data_32s(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_SET_MASTER:
    Audio.recv_bridge_set_master(msg->payload, msg->payload_size);
    break;
  case CMD_AUDIO_DISABLE:
    Audio.set_enable(false);
    break;
  case CMD_VIBRATION_ENABLE:
    Vibration.enable(true);
    break;
  case CMD_VIBRATION_DATA:
    Vibration.recv_bridge_data(msg->payload, msg->payload_size);
    break;
  case CMD_VIBRATION_DISABLE:
    Vibration.enable(false);
    break;
  default:
    break;
  }
}
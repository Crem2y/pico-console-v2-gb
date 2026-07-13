#pragma once

enum bridge_cmd {
  // General commands
  CMD_NONE              = 0x00,
  CMD_HW_INFO_REQ       = 0x01,
  CMD_HW_INFO_RES       = 0x02, // [version_L][version_H][support_flag1][support_flag2] ...
  CMD_HW_NAME_REQ       = 0x03,
  CMD_HW_NAME_RES       = 0x04, // [friendly name (max 16 bytes)]
  CMD_SW_INFO_REQ       = 0x05,
  CMD_SW_INFO_RES       = 0x06, // [version 2bytes][date, time 6bytes][support_flag 4bytes] ...

  CMD_PING              = 0x0F,

  // Southbridge commands
  CMD_TEMPERATURE_DATA  = 0x11, // [temp1_L][temp1_H][temp2_L][temp2_H]...
  CMD_POWER_STATUS      = 0x12, // [vin_mv_L][vin_mv_H]
  CMD_POWER_CONTROL     = 0x13, // [data1] ...
  CMD_BATTERY_STATUS    = 0x14, // [vbat_mv_L][vbat_mv_H][percent_x10_L][percent_x10_H][status_flag][fault_flag]
  CMD_BATTERY_CONTROL   = 0x15, // [data1] ...

  CMD_GAMEPAD_ENABLE    = 0x20, // [flags]
  CMD_GAMEPAD_DATA      = 0x21, // [btn_l][btn_m][btn_h][joyLx][joyLy][joyRx][joyRy]
  CMD_GAMEPAD_RAW_DATA  = 0x22, // [axle1_L][axle1_H][axle2_L][axle2_H] ...
  CMD_GAMEPAD_CALI_DATA = 0x23, // [channel][data_L][data_H][channel][data_L][data_H] ...

  CMD_IR_RX_ENABLE      = 0x30, // [format]
  CMD_IR_RX_DATA        = 0x31, // [format][sequencing][data1][data2]...
  CMD_IR_RX_DISABLE     = 0x3F,

  CMD_IR_TX_ENABLE      = 0x40, // [format]
  CMD_IR_TX_DATA        = 0x41, // [format][sequencing][data1][data2]...
  CMD_IR_TX_DISABLE     = 0x4F,

  CMD_IMU_ENABLE        = 0x50, // [flags]
  CMD_IMU_DATA          = 0x51,
  CMD_IMU_ACCEL_DATA    = 0x52, // [ax][ay][az]
  CMD_IMU_GYRO_DATA     = 0x53, // [gx][gy][gz]
  CMD_IMU_MAG_DATA      = 0x54, // [mx][my][mz]
  CMD_IMU_DISABLE       = 0x5F,

  CMD_AUDIO_ENABLE      = 0x60,
  CMD_AUDIO_NOTE_DATA   = 0x61, // [ch][freq][vol]
  CMD_AUDIO_SET_WAVE    = 0x62, // [ch][wav]
  CMD_AUDIO_SET_ENV     = 0x63, // [ch][tick_us_32][step]
  CMD_AUDIO_SET_MASTER  = 0x64, // [vol]
  CMD_AUDIO_SET_PIT_ENV = 0x65, // [ch][tick_us_32][target_semitones][step]
  CMD_AUDIO_DISABLE     = 0x6F,

  CMD_VIBRATION_ENABLE  = 0x70,
  CMD_VIBRATION_DATA    = 0x71, // [Lfreql][Lfreqh][Lpwr][Rfreql][Rfreqh][Rpwr]
  CMD_VIBRATION_DISABLE = 0x7F,

  // Rf bridge commands
  CMD_LED_CONTROL       = 0x11, // [LED1][LED2][LED3][LED4]
};
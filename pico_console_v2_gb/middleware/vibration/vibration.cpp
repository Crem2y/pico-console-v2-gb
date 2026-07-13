#include "vibration.hpp"
#include "bridge_protocol.hpp"

extern bridgeProtocol Bridge;

vibration::vibration(void) {
}

void vibration::init(void) {
  freq[VIBRATION_L] = 0;
  power[VIBRATION_L] = 0;
  freq[VIBRATION_R] = 0;
  power[VIBRATION_R] = 0;
}

void vibration::set_vibration(enum vibration_channel channel, uint16_t freq, uint8_t power) {
  if(channel > VIBRATION_R) return;
  this->freq[channel] = freq;
  this->power[channel] = power;
}

void vibration::update(void) {
  if(prev_freq[VIBRATION_L] == freq[VIBRATION_L] &&
     prev_freq[VIBRATION_R] == freq[VIBRATION_R] &&
     prev_power[VIBRATION_L] == power[VIBRATION_L] &&
     prev_power[VIBRATION_R] == power[VIBRATION_R]) {
    return;
  }

  int payload_size = 6;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = freq[VIBRATION_L] & 0xFF;         // low byte of channel 1 frequency
  payload_buf[1] = (freq[VIBRATION_L] >> 8) & 0xFF;  // high byte of channel 1 frequency
  payload_buf[2] = power[VIBRATION_L];               // channel 1 power
  payload_buf[3] = freq[VIBRATION_R] & 0xFF;         // low byte of channel 2 frequency
  payload_buf[4] = (freq[VIBRATION_R] >> 8) & 0xFF;  // high byte of channel 2 frequency
  payload_buf[5] = power[VIBRATION_R];               // channel 2 power

  Bridge.send(CMD_VIBRATION_DATA, payload_size, payload_buf);

  prev_freq[VIBRATION_L] = freq[VIBRATION_L];
  prev_freq[VIBRATION_R] = freq[VIBRATION_R];
  prev_power[VIBRATION_L] = power[VIBRATION_L];
  prev_power[VIBRATION_R] = power[VIBRATION_R];
}
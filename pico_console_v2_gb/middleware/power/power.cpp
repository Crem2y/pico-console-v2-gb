#include "power.hpp"
#include "bridge_protocol.hpp"

extern bridgeProtocol Bridge;

power::power(void) {

}

void power::init(void) {
  input_voltage = 0.0f;
}

void power::update(void) {

}

void power::recv_bridge_power_status(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 2) return;

  uint16_t voltage_mv = (payload[1] << 8) | payload[0];
  input_voltage = (float)voltage_mv / 1000;
}
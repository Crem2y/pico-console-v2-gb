#include "charger.hpp"
#include "bridge_protocol.hpp"

extern bridgeProtocol Bridge;

charger::charger(void) {

}

void charger::init(void) {
  voltage = 0.0f;
  level = 0.0f;

  is_battery_exist = false;
  charging = false;
  fault = 0x00;
}

float charger::get_bat_voltage(void) {
  return voltage;
}

float charger::get_bat_level(void) {
  return level;
}

bool charger::get_battery_exist(void) {
  return is_battery_exist;
}

bool charger::get_charging_status(void) {
  return charging;
}

uint8_t charger::get_fault_status(void) {
  return fault;
}

void charger::send_bridge_bat_control(void) {
  
}

void charger::recv_bridge_bat_status(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 6) return;

  uint16_t voltage_mv = (payload[1] << 8) | payload[0];
  voltage = (float)voltage_mv / 1000;

  uint16_t level_x10 = (payload[3] << 8) | payload[2];
  level = (float)level_x10 / 10;

  uint8_t status_flag = payload[4];
  is_battery_exist = (status_flag & 0x80);
  charging = (status_flag & 0x40);

  fault = payload[5];
}
#include "bridge_control.hpp"
#include "bridge_protocol.hpp"

bridgeControl::bridgeControl(bridgeProtocol* bridge) {
  Bridge = bridge;
}

void bridgeControl::init(void) {
  connected = false;
  last_bridge_comm_time = 0;

  info.hw_ver = 0;
  info.hw_support = 0;
  info.hw_name[0] = 0;
  info.sw_ver = 0;
  info.sw_support = 0;
}

void bridgeControl::update(void) {
  connected = (system_time_elapsed_ms(get_system_time_ms(), last_bridge_comm_time) < 500);
}

void bridgeControl::reset_bridge(void) {

}

void bridgeControl::send_bridge_hw_info_req(void) {
  Bridge->send(CMD_HW_INFO_REQ, 0, NULL);
}

void bridgeControl::send_bridge_hw_name_req(void) {
  Bridge->send(CMD_HW_NAME_REQ, 0, NULL);
}

void bridgeControl::send_bridge_sw_info_req(void) {
  Bridge->send(CMD_SW_INFO_REQ, 0, NULL);
}

void bridgeControl::recv_bridge_hw_info_res(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 6) return;

  memcpy(&info.hw_ver, &payload[0], sizeof(uint16_t));
  memcpy(&info.hw_support, &payload[2], sizeof(uint32_t));
}

void bridgeControl::recv_bridge_hw_name_res(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 1) return;

  //strncpy(info.hw_name, (char*)&payload[0], payload_size);
  memcpy(info.hw_name, &payload[0], payload_size);
}

void bridgeControl::recv_bridge_sw_info_res(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 12) return;

  memcpy(&info.sw_ver, &payload[0], sizeof(uint16_t));
  memcpy(&info.build_date, &payload[2], 3);
  memcpy(&info.build_time, &payload[5], 3);
  memcpy(&info.sw_support, &payload[8], sizeof(uint32_t));
}
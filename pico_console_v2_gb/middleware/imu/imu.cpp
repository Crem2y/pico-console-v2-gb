#include "imu.hpp"
#include "bridge_protocol.hpp"

extern bridgeProtocol Bridge;

imu::imu(void) {

}

void imu::init(void) {

}

void imu::update(void) {

}

float imu::get_accel_x() {
  return accel_x;
}

float imu::get_accel_y() {
  return accel_y;
}

float imu::get_accel_z() {
  return accel_z;
}

float imu::get_gyro_x() {
  return gyro_x;
}

float imu::get_gyro_y() {
  return gyro_y;
}

float imu::get_gyro_z() {
  return gyro_z;
}

void imu::send_bridge_enable(bool enable_accel, bool enable_gyro) {
  int payload_size = 1;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = (enable_accel ? 1 : 0) | (enable_gyro ? 2 : 0);

  Bridge.send(CMD_IMU_ENABLE, 1, payload_buf);
}

void imu::send_bridge_disable(void) {
  Bridge.send(CMD_IMU_DISABLE, 0, NULL);
}

void imu::recv_bridge_accel_data(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 12) return;

  memcpy(&accel_x, &payload[0], sizeof(float));
  memcpy(&accel_y, &payload[4], sizeof(float));
  memcpy(&accel_z, &payload[8], sizeof(float));
}

void imu::recv_bridge_gyro_data(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < 12) return;

  memcpy(&gyro_x, &payload[0], sizeof(float));
  memcpy(&gyro_y, &payload[4], sizeof(float));
  memcpy(&gyro_z, &payload[8], sizeof(float));
}
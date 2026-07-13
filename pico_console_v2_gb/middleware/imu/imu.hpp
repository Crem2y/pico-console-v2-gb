#pragma once

#include <stdint.h>
#include <string.h>

class imu {
  public:
    imu(void);

    void init(void);
    void update(void);

    void set_enable(bool enable_accel, bool enable_gyro) {
      if(enable_accel || enable_gyro) {
        send_bridge_enable(enable_accel, enable_gyro);
      } else {
        send_bridge_disable();
      }
    }

    float get_accel_x();
    float get_accel_y();
    float get_accel_z();

    float get_gyro_x();
    float get_gyro_y();
    float get_gyro_z();

    void send_bridge_enable(bool enable_accel, bool enable_gyro);
    void send_bridge_disable(void);

    void recv_bridge_accel_data(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_gyro_data(const uint8_t* payload, uint8_t payload_size);

  private:
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
};
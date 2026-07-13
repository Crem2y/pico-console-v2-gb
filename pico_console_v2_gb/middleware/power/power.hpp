#pragma once

#include <stdint.h>

class power {
  public:
    power(void);

    void init(void);
    void update(void);

    float get_input_voltage(void) {
      return input_voltage;
    }

    void recv_bridge_power_status(const uint8_t* payload, uint8_t payload_size);

  private:
    float input_voltage;
};

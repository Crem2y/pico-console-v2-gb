#pragma once

#include <stdint.h>

class charger {
  public:
    charger(void);

    void init(void);
    void update(void);

    float get_bat_voltage(void);
    float get_bat_level(void);

    bool get_battery_exist(void);
    bool get_charging_status(void);
    uint8_t get_fault_status(void);

    void send_bridge_bat_control(void);

    void recv_bridge_bat_status(const uint8_t* payload, uint8_t payload_size);

  private:
    float voltage;
    float level;

    bool is_battery_exist;
    bool charging;
    uint8_t fault;
};

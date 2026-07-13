#pragma once

#include <stdint.h>
#include "system_time.h"
#include "bridge_protocol.hpp"

typedef struct _bridge_info_t {
  uint16_t hw_ver;
  uint16_t sw_ver;
  uint32_t build_date;
  uint32_t build_time;
  uint32_t hw_support;
  uint32_t sw_support;
  char hw_name[16];
} bridge_info_t;

class bridgeControl {
  public:
    bool connected;
    bridge_info_t info;

    bridgeControl(bridgeProtocol* bridge);

    void init(void);
    void update(void);
    void update_last_comm_time(void) {
      last_bridge_comm_time = get_system_time_ms();
    }

    void reset_bridge(void);

    void read_hw_info(void) {
      send_bridge_hw_info_req();
    }
    void read_hw_name(void) {
      send_bridge_hw_name_req();
    }
    void read_sw_info(void) {
      send_bridge_sw_info_req();
    }

    void send_bridge_hw_info_req(void);
    void send_bridge_hw_name_req(void);
    void send_bridge_sw_info_req(void);

    void recv_bridge_hw_info_res(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_hw_name_res(const uint8_t* payload, uint8_t payload_size);
    void recv_bridge_sw_info_res(const uint8_t* payload, uint8_t payload_size);

  private:
    time_ms_t last_bridge_comm_time;
    bridgeProtocol* Bridge;
};

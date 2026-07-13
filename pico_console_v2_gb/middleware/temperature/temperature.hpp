#pragma once

#include "temp_builtin.h"

#define TEMP_CH_NUM 3

enum temp_ch {
  TEMP_BUILTIN = 0,
  TEMP_SOUTHBRIDGE,
  TEMP_NTC
};

class temperature {
  public:
    temperature(void);

    void init(void);
    void update(void);
    void recv_bridge_data(const uint8_t* payload, uint8_t payload_size);

    float get_temp(temp_ch ch);

  private:
    float temp[TEMP_CH_NUM];
};
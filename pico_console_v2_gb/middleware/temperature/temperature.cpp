#include "temperature.hpp"

temperature::temperature(void) {
  for(int i=0; i<TEMP_CH_NUM; i++) {
    temp[i] = 0.0;
  }
}

void temperature::init(void) {
  built_in_temp_init();
}

void temperature::update(void) {
  temp[TEMP_BUILTIN] = built_in_temp_read();
}

void temperature::recv_bridge_data(const uint8_t* payload, uint8_t payload_size) {
  if(payload_size < (TEMP_CH_NUM-1) * 2) return; // each temp is 2 bytes (int16_t)

  int16_t temp_x100;
  for(int i=0; i<TEMP_CH_NUM-1; i++) {
    temp_x100 = ((payload[i*2+1] << 8) | payload[i*2]);
    temp[i+1] = (float)temp_x100 / 100.0f;
  }
}

float temperature::get_temp(temp_ch ch) {
  if(ch < TEMP_CH_NUM) {
    return temp[ch];
  } else {
    return 0;
  }
}
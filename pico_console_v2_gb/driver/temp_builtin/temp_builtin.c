#include "temp_builtin.h"

void built_in_temp_init(void) {
  adc_init();
}

float built_in_temp_read(void) {
  adc_set_temp_sensor_enabled(true);
  adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM);

  /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
  const float conversionFactor = 3.3f / (1 << 12);

  float adc = (float)adc_read() * conversionFactor;
  float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

  return tempC;
}
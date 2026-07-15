#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void audio_init(uint8_t default_volume);
void audio_write(const uint16_t addr, const uint8_t val);
uint8_t audio_read(const uint16_t addr);

#ifdef __cplusplus
}
#endif
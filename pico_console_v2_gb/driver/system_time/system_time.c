#include "system_time.h"

inline time_us_t get_system_time_us(void) {
    return to_us_since_boot(get_absolute_time());
}

inline time_ms_t get_system_time_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

inline time_us_t system_time_elapsed_us(time_us_t now, time_us_t since) {
    return (time_us_t)(now - since);
}

inline time_ms_t system_time_elapsed_ms(time_ms_t now, time_ms_t since) {
    return (time_ms_t)(now - since);
}
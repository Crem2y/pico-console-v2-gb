#pragma once

#include <stdint.h>
#include <stddef.h>

#define IR_LINK_MAX_DATA_SIZE 224 // (16-2)*16 = 224 - 1 byte for format - 1 byte for sequencing

enum ir_format {
  IR_FORMAT_MANUAL  = 0x00,
  IR_FORMAT_NEC     = 0x01,
  IR_FORMAT_UNKNOWN = 0xFF,
};

class irLink {
  public:
    uint8_t tx_data_buf[IR_LINK_MAX_DATA_SIZE];
    int tx_data_len;
    uint8_t rx_data_buf[IR_LINK_MAX_DATA_SIZE];
    int rx_data_len;
    enum ir_format rx_format, rx_data_format;

    irLink(void);

    void init(void);

    inline void set_enable_tx(bool enable_tx, enum ir_format format) {
      send_bridge_enable_tx(enable_tx, format);
    }
    inline void set_enable_rx(bool enable_rx, enum ir_format format) {
      send_bridge_enable_rx(enable_rx, format);
    }

    enum ir_format get_rx_format(void);
    enum ir_format get_rx_data_format(void);
    bool is_data_ready(void);
    int get_raw_data_pulses(void);

    void recv_bridge_rx_data(const uint8_t* payload, size_t payload_size);

    void send_bridge_enable_tx(bool enable, enum ir_format format);
    void send_bridge_enable_rx(bool enable, enum ir_format format);
    void send_bridge_tx_data(enum ir_format format, const uint8_t* data, size_t len);
  private:
    bool is_rx_data_ready;
};
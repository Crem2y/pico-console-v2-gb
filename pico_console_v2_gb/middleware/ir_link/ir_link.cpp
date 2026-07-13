#include "ir_link.hpp"
#include "bridge_protocol.hpp"

extern bridgeProtocol Bridge;

irLink::irLink(void) {

}

void irLink::init(void) {
  rx_data_format = IR_FORMAT_UNKNOWN;
  rx_format = IR_FORMAT_UNKNOWN;
}

enum ir_format irLink::get_rx_format(void) {
  return rx_format;
}

enum ir_format irLink::get_rx_data_format(void) {
  return rx_data_format;
}

bool irLink::is_data_ready(void) {
  return rx_data_len > 0;
}

int irLink::get_raw_data_pulses(void) {
  if(rx_format != IR_FORMAT_MANUAL) return -1;

  // each pulse represented by 2 bytes in manual format
  return rx_data_len / 2;
}

void irLink::send_bridge_enable_tx(bool enable, enum ir_format format) {
  int payload_size = 1;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = (uint8_t)format;
  if (enable) {
    Bridge.send(CMD_IR_TX_ENABLE, payload_size, payload_buf);
  } else {
    Bridge.send(CMD_IR_TX_DISABLE, payload_size, payload_buf);
  }
}

void irLink::send_bridge_enable_rx(bool enable, enum ir_format format) {
  int payload_size = 1;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  payload_buf[0] = (uint8_t)format;
  if (enable) {
    Bridge.send(CMD_IR_RX_ENABLE, payload_size, payload_buf);
  } else {
    Bridge.send(CMD_IR_RX_DISABLE, payload_size, payload_buf);
  }

  rx_format = (enum ir_format)format;
}

void irLink::send_bridge_tx_data(enum ir_format format, const uint8_t* data, size_t len) {
  if(len > IR_LINK_MAX_DATA_SIZE) {
    len = IR_LINK_MAX_DATA_SIZE; // Limit to max data size
  }
  size_t payload_size;
  uint8_t payload_buf[PAYLOAD_MAX_SIZE];

  const size_t data_per_packet = (PAYLOAD_MAX_SIZE - 2);
  const size_t sequence_length = len / data_per_packet + ((len % data_per_packet) != 0 ? 1 : 0);

  for(int sequence_index = 0; sequence_index < sequence_length; sequence_index++) {
    size_t remain = len - (sequence_index * data_per_packet);
    size_t send_count = remain > data_per_packet ? data_per_packet : remain;

    payload_buf[0] = (uint8_t)format;
    payload_buf[1] = (sequence_index << 4) | ((sequence_length-1) & 0x0F);

    for (size_t j = 0; j < send_count; j++) {
      payload_buf[2 + j] = data[sequence_index * data_per_packet + j];
    }

    size_t payload_size = 2 + send_count;
    Bridge.send(CMD_IR_TX_DATA, payload_size, payload_buf);
  }
}

void irLink::recv_bridge_rx_data(const uint8_t* payload, size_t payload_size) {
  if(payload_size < 2) return; // Not enough data

  rx_data_format = (enum ir_format)payload[0];
  uint8_t sequence_info = payload[1];
  size_t sequence_index = (sequence_info >> 4) & 0x0F;
  size_t sequence_length = sequence_info & 0x0F;

  if(sequence_index == 0) {
    rx_data_len = 0;  // reset data length for new sequence
  }

  for(int i = 0; i < payload_size - 2; i++) {
    rx_data_buf[rx_data_len + i] = payload[i + 2];
  }
  rx_data_len += (payload_size - 2);

  if(sequence_index != sequence_length) {
    return; // wait for more sequences
  }
}
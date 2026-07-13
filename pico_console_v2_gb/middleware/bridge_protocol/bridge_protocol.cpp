#include "bridge_protocol.hpp"

bridgeProtocol::bridgeProtocol(void) {

}

void bridgeProtocol::init(void) {
  cmd_handler = NULL;
  readable = NULL;
  read = NULL;
  writable = NULL;
  write = NULL;

  parse_seq = SEQ_WAIT_HEADER;
  parse_payload_index = 0;
  parse_checksum = 0;

  tx_msg_queue.head = 0;
  tx_msg_queue.tail = 0;

  rx_msg_queue.head = 0;
  rx_msg_queue.tail = 0;
}

int bridgeProtocol::send(enum bridge_cmd cmd, size_t payload_size, const uint8_t* payload) {
  return push_tx(make_msg(cmd, payload_size, payload));
}

bridge_msg_t bridgeProtocol::make_msg(enum bridge_cmd cmd, size_t payload_size, const uint8_t* payload) {
  bridge_msg_t msg;
  msg.cmd = (uint8_t)cmd;
  memset(msg.payload, 0, PAYLOAD_MAX_SIZE);
  if (payload_size > PAYLOAD_MAX_SIZE) {
    payload_size = PAYLOAD_MAX_SIZE;
  }
  if(payload != NULL) {
    memcpy(msg.payload, payload, payload_size);
  } else {
    payload_size = 0;
  }
  msg.payload_size = payload_size;

  return msg;
}

int bridgeProtocol::push_tx(bridge_msg_t msg) {
  size_t next_tail = (tx_msg_queue.tail + 1) % BRIDGE_MSG_QUEUE_SIZE;
  if (next_tail == tx_msg_queue.head) {
    return -1; // Queue is full
  }
  tx_msg_queue.queue[tx_msg_queue.tail] = msg;
  tx_msg_queue.tail = next_tail;
  return 1; // Success
}

int bridgeProtocol::pop_tx(bridge_msg_t* msg) {
  if (tx_msg_queue.head == tx_msg_queue.tail) {
    return -1; // Queue is empty
  }
  *msg = tx_msg_queue.queue[tx_msg_queue.head];
  tx_msg_queue.head = (tx_msg_queue.head + 1) % BRIDGE_MSG_QUEUE_SIZE;
  return 1; // Success
}

int bridgeProtocol::push_rx(bridge_msg_t msg) {
  size_t next_tail = (rx_msg_queue.tail + 1) % BRIDGE_MSG_QUEUE_SIZE;
  if (next_tail == rx_msg_queue.head) {
    return -1; // Queue is full
  }
  rx_msg_queue.queue[rx_msg_queue.tail] = msg;
  rx_msg_queue.tail = next_tail;
  return 1; // Success
}

int bridgeProtocol::pop_rx(bridge_msg_t* msg) {
  if (rx_msg_queue.head == rx_msg_queue.tail) {
    return -1; // Queue is empty
  }
  *msg = rx_msg_queue.queue[rx_msg_queue.head];
  rx_msg_queue.head = (rx_msg_queue.head + 1) % BRIDGE_MSG_QUEUE_SIZE;
  return 1; // Success
}

void bridgeProtocol::parse_bytes(const uint8_t* data, size_t data_size) {
  for (size_t i = 0; i < data_size; i++) {
    uint8_t byte = data[i];
    switch (parse_seq) {
      case SEQ_WAIT_HEADER:
        if (byte == BRIDGE_HEADER) {
          parse_seq = SEQ_WAIT_COMMAND;
        } else {
          // Invalid header
          print_error(parse_seq, byte);
        }
        break;
      case SEQ_WAIT_COMMAND:
        parse_msg.cmd = byte;
        parse_checksum = byte;
        parse_seq = SEQ_WAIT_LENGTH;
        break;
      case SEQ_WAIT_LENGTH:
        parse_msg.payload_size = byte;
        parse_checksum ^= byte;
        if (parse_msg.payload_size > PAYLOAD_MAX_SIZE) {
          // Invalid payload size, reset state
          print_error(parse_seq, byte);
          parse_seq = SEQ_WAIT_HEADER;
        } else if (parse_msg.payload_size == 0) {
          parse_seq = SEQ_WAIT_INTEGRITY; // No payload, skip to integrity check
        } else {
          parse_payload_index = 0;
          parse_seq = SEQ_WAIT_PAYLOAD;
        }
        break;
      case SEQ_WAIT_PAYLOAD:
        parse_msg.payload[parse_payload_index++] = byte;
        parse_checksum ^= byte;
        if (parse_payload_index >= parse_msg.payload_size) {
          parse_seq = SEQ_WAIT_INTEGRITY;
        }
        break;
      case SEQ_WAIT_INTEGRITY: {
        uint8_t packet_checksum = byte;
        if (parse_checksum == packet_checksum) {
          parse_seq = SEQ_WAIT_TAIL;
        } else {
          // Invalid checksum, reset state
          //printf("Bridge protocol error: expected 0x%02X, got 0x%02X\n", checksum, packet_checksum);
          //bridge_protocol_error_handle(seq, byte);
          parse_seq = SEQ_WAIT_HEADER;
        }
        break;
      }
      case SEQ_WAIT_TAIL:
        if (byte == BRIDGE_TAIL) {
          //printf("Bridge protocol OK! (cmd: 0x%02X, size: %d)\n", cmd.cmd, cmd.payload_size);
          push_rx(parse_msg);
        } else {
          // Invalid tail
          print_error(parse_seq, byte);
        }
        // Reset state for next command
        parse_seq = SEQ_WAIT_HEADER;
        break;
    }
  }
}

void bridgeProtocol::dispatch_rx(void) {
  bridge_msg_t msg;
  while(pop_rx(&msg) > 0) {
    if(cmd_handler) {
      cmd_handler(&msg);
    }
  }
}

inline void bridgeProtocol::print_error(ProtocolSequence error_seq, uint8_t data) {
  /*
  // print error message
  switch (error_seq)
  {
  case SEQ_WAIT_HEADER:
    printf("Bridge protocol error: expected header 0x%02X, got 0x%02X\n", BRIDGE_HEADER, data);
    break;
  case SEQ_WAIT_COMMAND:
    break;
  case SEQ_WAIT_LENGTH:
    printf("Bridge protocol error: expected length < %d, got 0x%02X\n", PAYLOAD_MAX_SIZE, data);
    break;
  case SEQ_WAIT_PAYLOAD:
    break;
  case SEQ_WAIT_INTEGRITY:
    printf("Bridge protocol error: expected integrity byte, got 0x%02X\n", data);
    break;
  case SEQ_WAIT_TAIL:
    printf("Bridge protocol error: expected tail 0x%02X, got 0x%02X\n", BRIDGE_TAIL, data);
    break;
  default:
    break;
  }
  */
}

void bridgeProtocol::set_cmd_handler(void (*do_cmd)(const bridge_msg_t*)) {
  cmd_handler = do_cmd;
}

void bridgeProtocol::set_transport_handler(bridge_transport_t* handlers) {
  readable = handlers->readable;
  read = handlers->read;
  writable = handlers->writable;
  write = handlers->write;
}

bridge_protocol_t bridgeProtocol::encode_packet(const bridge_msg_t* msg) {
  bridge_protocol_t packet;
  size_t payload_size = msg->payload_size;

  packet.header = BRIDGE_HEADER;
  packet.cmd = (uint8_t)msg->cmd;
  memset(packet.payload, 0, PAYLOAD_MAX_SIZE);
  if (payload_size > PAYLOAD_MAX_SIZE) {
    payload_size = PAYLOAD_MAX_SIZE;
  }
  if(msg->payload != NULL) {
    memcpy(packet.payload, msg->payload, payload_size);
  } else {
    payload_size = 0;
  }
  packet.payload_size = payload_size;
  packet.checksum = packet.cmd;
  packet.checksum ^= packet.payload_size;
  for (size_t i = 0; i < packet.payload_size; i++) {
    packet.checksum ^= packet.payload[i];
  }
  packet.tail = BRIDGE_TAIL;

  return packet;
}

void bridgeProtocol::process_io(void) {
  if(readable == NULL || read == NULL || writable == NULL || write == NULL) {
    return;
  }

  if(readable() > 0) {
    uint8_t data[128];
    int rx_data_size = read(data, 128);
    parse_bytes(data, rx_data_size);
  }

  bridge_msg_t msg;
  bridge_protocol_t packet;
  if(pop_tx(&msg) > 0) {
    packet = encode_packet(&msg);
    uint8_t buffer[sizeof(bridge_protocol_t)];
    buffer[0] = packet.header;
    buffer[1] = packet.cmd;
    buffer[2] = packet.payload_size;
    memcpy(&buffer[3], packet.payload, packet.payload_size);
    buffer[3 + packet.payload_size] = packet.checksum;
    buffer[4 + packet.payload_size] = packet.tail;

    int tx_data_size = 5 + packet.payload_size;
    if(writable() >= tx_data_size) {
      write(buffer, tx_data_size);
    }
  }
}
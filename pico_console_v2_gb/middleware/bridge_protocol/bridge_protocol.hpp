#pragma once

#include <stdint.h>
#include <string.h>
#include "bridge_commands.h"

#define BRIDGE_MSG_QUEUE_SIZE 32
#define PAYLOAD_MAX_SIZE 16
#define BRIDGE_HEADER 0xAA
#define BRIDGE_TAIL 0x55

typedef struct _bridge_msg_t {
  uint8_t cmd;                        // command code
  uint8_t payload_size;               // max 16
  uint8_t payload[PAYLOAD_MAX_SIZE];  // max payload size is 16 bytes
} bridge_msg_t;

typedef struct _bridge_msg_queue_t {
  bridge_msg_t queue[BRIDGE_MSG_QUEUE_SIZE];
  size_t head;
  size_t tail;
} bridge_msg_queue_t;

typedef struct _bridge_protocol_t {
  uint8_t header;                     // 0xAA
  uint8_t cmd;                        // command code
  uint8_t payload_size;               // max 16
  uint8_t payload[PAYLOAD_MAX_SIZE];  // max payload size is 16 bytes
  uint8_t checksum;                   // XOR of cmd and payload
  uint8_t tail;                       // 0x55
} bridge_protocol_t;

typedef struct _bridge_transport_t {  
  int (*readable)(void);  
  int (*read)(uint8_t*, size_t);  
  int (*writable)(void);  
  int (*write)(const uint8_t*, size_t);  
} bridge_transport_t;

typedef enum {
  SEQ_WAIT_HEADER = 0,
  SEQ_WAIT_COMMAND,
  SEQ_WAIT_LENGTH,
  SEQ_WAIT_PAYLOAD,
  SEQ_WAIT_INTEGRITY,
  SEQ_WAIT_TAIL
} ProtocolSequence;

class bridgeProtocol {
  public:
    bridgeProtocol(void);

    void init(void);

    int send(enum bridge_cmd cmd, size_t payload_size, const uint8_t* payload);

    void process_io(void);
    void dispatch_rx(void);
    void set_cmd_handler(void (*do_cmd)(const bridge_msg_t*));
    void set_transport_handler(bridge_transport_t* handlers);

  private:
    ProtocolSequence parse_seq;
    bridge_msg_t parse_msg;
    bridge_protocol_t parse_cmd;
    size_t parse_payload_index;
    uint8_t parse_checksum;

    bridge_msg_queue_t tx_msg_queue;
    bridge_msg_queue_t rx_msg_queue;

    void parse_bytes(const uint8_t* data, size_t data_size);

    bridge_msg_t make_msg(enum bridge_cmd cmd, size_t payload_size, const uint8_t* payload);

    int push_tx(bridge_msg_t msg);
    int pop_tx(bridge_msg_t* msg);

    int push_rx(bridge_msg_t msg);
    int pop_rx(bridge_msg_t* msg);

    inline void print_error(ProtocolSequence error_seq, uint8_t data);
    bridge_protocol_t encode_packet(const bridge_msg_t* msg);

    void (*cmd_handler)(const bridge_msg_t*);
    int (*readable)(void);  
    int (*read)(uint8_t* buf, size_t max_size);  
    int (*writable)(void);  
    int (*write)(const uint8_t* data, size_t size);  
};
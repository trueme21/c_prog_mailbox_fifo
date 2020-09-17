#ifndef NDP_ENV_H_
#define NDP_ENV_H_

// Refer to excel file
#define MBOX_START_ADDR 0xA0020000
#define QUERY_START_POINT 0x800000000
#define OUTPUT_START_ADDR 0x80028C000
#define EMB_START_ADDR 0x8002CD000

#define MBOX_SIZE 64  // TODO
#define OUTPUT_SIZE 262144
// EMB_SIZE is defined at initialization

// opcode
#define OP_INIT 0x01
#define OP_FWD 0x02
#define OP_BWD 0x03
#define OP_KILL 0x04
#define OP_INIT_CPY 0x05

#define MBOX_EMPTY 0x01

#endif


#ifndef IR_RX_H
#define IR_RX_H

#include <stdint.h>
#include <stdbool.h>
#include "storage.h"

void ir_rx_init(void);
bool ir_rx_capture(ir_code_t *out, uint16_t timeout_ms);

#endif

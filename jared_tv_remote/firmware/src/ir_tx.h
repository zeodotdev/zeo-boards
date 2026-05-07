#ifndef IR_TX_H
#define IR_TX_H

#include <stdint.h>
#include "storage.h"

void ir_tx_init(void);
void ir_tx_send(const ir_code_t *code);

#endif

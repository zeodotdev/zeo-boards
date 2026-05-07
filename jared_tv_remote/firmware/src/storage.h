#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#define IR_MAX_EDGES   70
#define IR_TICK_US_DEF 100

typedef struct {
    uint8_t edge_count;
    uint8_t tick_us;
    uint8_t edges[IR_MAX_EDGES];
} ir_code_t;

void storage_init(void);
void storage_format(void);

bool storage_load(uint8_t slot, ir_code_t *out);
bool storage_save(uint8_t slot, const ir_code_t *in);

#endif

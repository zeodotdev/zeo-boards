#include "storage.h"
#include "board.h"
#include <avr/eeprom.h>
#include <string.h>

#define MAGIC0  'T'
#define MAGIC1  'V'
#define MAGIC2  'R'
#define MAGIC3  'M'
#define VERSION 1

#define HDR_SIZE   16
#define SLOT_SIZE  (sizeof(ir_code_t))  /* 2 + 70 = 72 */

_Static_assert(SLOT_SIZE == 72, "ir_code_t must be 72 bytes");
_Static_assert(HDR_SIZE + NUM_CODES * SLOT_SIZE <= 1024, "EEPROM layout too big");

static uint8_t  EEMEM ee_header[HDR_SIZE];
static ir_code_t EEMEM ee_slots[NUM_CODES];

static bool header_valid(void) {
    uint8_t hdr[5];
    eeprom_read_block(hdr, ee_header, 5);
    return hdr[0] == MAGIC0 && hdr[1] == MAGIC1 &&
           hdr[2] == MAGIC2 && hdr[3] == MAGIC3 &&
           hdr[4] == VERSION;
}

void storage_format(void) {
    ir_code_t empty;
    memset(&empty, 0, sizeof(empty));
    for (uint8_t i = 0; i < NUM_CODES; i++) {
        eeprom_update_block(&empty, &ee_slots[i], sizeof(empty));
    }
    uint8_t hdr[HDR_SIZE] = { MAGIC0, MAGIC1, MAGIC2, MAGIC3, VERSION };
    eeprom_update_block(hdr, ee_header, HDR_SIZE);
}

void storage_init(void) {
    if (!header_valid()) storage_format();
}

bool storage_load(uint8_t slot, ir_code_t *out) {
    if (slot >= NUM_CODES || !out) return false;
    eeprom_read_block(out, &ee_slots[slot], sizeof(*out));
    return out->edge_count > 0 && out->edge_count <= IR_MAX_EDGES && out->tick_us > 0;
}

bool storage_save(uint8_t slot, const ir_code_t *in) {
    if (slot >= NUM_CODES || !in) return false;
    if (in->edge_count == 0 || in->edge_count > IR_MAX_EDGES) return false;
    eeprom_update_block(in, &ee_slots[slot], sizeof(*in));
    return true;
}

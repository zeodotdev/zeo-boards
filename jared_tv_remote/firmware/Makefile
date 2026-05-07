MCU       = atmega32u4
F_CPU     = 16000000UL
TARGET    = tv_remote
BUILDDIR  = build
SRCDIR    = src

CC        = avr-gcc
OBJCOPY   = avr-objcopy
SIZE      = avr-size
DFU       = dfu-programmer

SRCS      = $(wildcard $(SRCDIR)/*.c)
OBJS      = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS      = $(OBJS:.o=.d)

CFLAGS    = -mmcu=$(MCU) -DF_CPU=$(F_CPU) \
            -Os -std=gnu11 -Wall -Wextra -Werror \
            -ffunction-sections -fdata-sections \
            -funsigned-char -funsigned-bitfields \
            -fpack-struct -fshort-enums \
            -MMD -MP

LDFLAGS   = -mmcu=$(MCU) -Wl,--gc-sections -Wl,-Map=$(BUILDDIR)/$(TARGET).map

.PHONY: all clean flash size erase reset dfu

all: $(BUILDDIR)/$(TARGET).hex size

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILDDIR)/$(TARGET).hex: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

$(BUILDDIR)/$(TARGET).eep: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom="alloc,load" \
		--change-section-lma .eeprom=0 $< $@ || true

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

size: $(BUILDDIR)/$(TARGET).elf
	$(SIZE) --mcu=$(MCU) -C $<

# Hold RESET while connecting USB (HWB is hardwired low) to enter the
# on-chip Atmel DFU bootloader, then release RESET and run `make flash`.
flash: $(BUILDDIR)/$(TARGET).hex
	$(DFU) $(MCU) erase --force
	$(DFU) $(MCU) flash $<
	$(DFU) $(MCU) launch

erase:
	$(DFU) $(MCU) erase --force

reset:
	$(DFU) $(MCU) launch

clean:
	rm -rf $(BUILDDIR)

-include $(DEPS)

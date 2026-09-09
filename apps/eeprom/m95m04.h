#ifndef M95M04_H
#define M95M04_H

#include "cy_pdl.h"
#include <stdint.h>
#include <stdbool.h>

#define EEPROM_CS_PORT     GPIO_PRT1
#define EEPROM_CS_PIN      3U

#define M95M04_CMD_WREN    0x06U
#define M95M04_CMD_WRDI    0x04U
#define M95M04_CMD_RDSR    0x05U
#define M95M04_CMD_READ    0x03U
#define M95M04_CMD_WRITE   0x02U

void m95m04_init(void);

uint8_t m95m04_read_status(void);
bool m95m04_write_enable(void);

bool m95m04_write_byte(uint32_t address, uint8_t data);
bool m95m04_read_byte(uint32_t address, uint8_t *data);

bool m95m04_test(void);

#endif
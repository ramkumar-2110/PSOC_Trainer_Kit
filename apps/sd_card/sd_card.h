#ifndef SD_CARD_H
#define SD_CARD_H

#include "cy_pdl.h"
#include <stdint.h>
#include <stdbool.h>

#define SD_CS_PORT      GPIO_PRT1
#define SD_CS_PIN       4U

#define SD_MOSI_PORT    GPIO_PRT1
#define SD_MOSI_PIN     0U

#define SD_MISO_PORT    GPIO_PRT1
#define SD_MISO_PIN     1U

#define SD_SCK_PORT     GPIO_PRT1
#define SD_SCK_PIN      2U

#define SD_CARD_TYPE_NONE   0U
#define SD_CARD_TYPE_SD1    1U
#define SD_CARD_TYPE_SD2    2U
#define SD_CARD_TYPE_SDHC   3U

bool sd_init(void);
bool sd_read_sector(uint32_t sector, uint8_t *buffer);
bool sd_write_sector(uint32_t sector, const uint8_t *buffer);

bool sd_is_ready(void);
uint8_t sd_get_type(void);

#endif
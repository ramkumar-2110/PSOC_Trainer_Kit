#ifndef SPI_DRV_H
#define SPI_DRV_H

#include <stdint.h>
#include <stdbool.h>

#include "cy_pdl.h"
#include "cybsp.h"

#define SPI_HW                  SCB0

#define SPI_CLK_DIV_TYPE        CY_SYSCLK_DIV_16_BIT
#define SPI_CLK_DIV_NUM         (3U)

#define SPI_MOSI_PORT           GPIO_PRT1
#define SPI_MOSI_PIN            (0U)

#define SPI_MISO_PORT           GPIO_PRT1
#define SPI_MISO_PIN            (1U)

#define SPI_SCK_PORT            GPIO_PRT1
#define SPI_SCK_PIN             (2U)

bool spi_drv_init(void);
uint8_t spi_drv_transfer(uint8_t data);

#endif
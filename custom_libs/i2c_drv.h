#ifndef I2C_DRV_H
#define I2C_DRV_H

#include <stdint.h>
#include <stdbool.h>

#include "cy_pdl.h"
#include "cybsp.h"

bool i2c_drv_init(void);

bool i2c_drv_write_bytes(
    uint8_t dev_addr,
    uint8_t control_byte,
    const uint8_t *data,
    uint16_t len
);

bool i2c_drv_read_bytes(
    uint8_t dev_addr,
    uint8_t *data,
    uint16_t len
);

bool i2c_drv_write_read_bytes(
    uint8_t dev_addr,
    uint8_t reg_addr,
    uint8_t *data,
    uint16_t len
);

#endif
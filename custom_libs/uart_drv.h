#ifndef UART_DRV_H
#define UART_DRV_H

#include <stdint.h>
#include <stdbool.h>

void uart_drv_init(void);
bool uart_drv_available(void);
uint8_t uart_drv_get(void);
void uart_printf(const char *format, ...);

#endif
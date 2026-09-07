#include "uart_drv.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include <stdio.h>
#include <stdarg.h>

void uart_drv_init(void)
{
    /*
     * UART_KP is already initialized by the generated
     * PSoC configuration.
     *
     * Do NOT call Cy_SCB_UART_Init() here.
     */
}

bool uart_drv_available(void)
{
    return (Cy_SCB_GetNumInRxFifo(UART_KP_HW) > 0U);
}

uint8_t uart_drv_get(void)
{
    return (uint8_t)Cy_SCB_UART_Get(UART_KP_HW);
}

void uart_printf(const char *format, ...)
{
    char buffer[128];

    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Cy_SCB_UART_PutString(UART_KP_HW, buffer);
}
#include "io_expander_app.h"

#include "cybsp.h"
#include "cycfg_peripherals.h"

#include <stdint.h>
#include <stdbool.h>


/* =========================================================
 * SHARED I2C CONTEXT
 * ========================================================= */

extern cy_stc_scb_i2c_context_t i2c_context;


/* =========================================================
 * CY8C9520A I/O EXPANDER
 * ========================================================= */

#define IO_EXPANDER_ADDR       0x20

#define REG_INPUT_PORT0        0x00
#define REG_INPUT_PORT1        0x01

#define REG_OUTPUT_PORT0       0x08
#define REG_OUTPUT_PORT1       0x09

#define REG_PORT_SELECT        0x18
#define REG_INVERSION          0x1B
#define REG_DIRECTION          0x1C
#define REG_DRIVE_PULLUP      0x1D
#define REG_DRIVE_STRONG      0x21

#define I2C_TIMEOUT             100


/* =========================================================
 * APPLICATION STATE
 * ========================================================= */

static bool io_expander_initialized = false;


/* =========================================================
 * I2C WRITE
 * ========================================================= */

static bool IO_Expander_Write(
    uint8_t reg,
    uint8_t data)
{
    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        IO_EXPANDER_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        I2C_TIMEOUT,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return false;
    }

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        reg,
        I2C_TIMEOUT,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            I2C_TIMEOUT,
            &i2c_context
        );

        return false;
    }

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        data,
        I2C_TIMEOUT,
        &i2c_context
    );

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        I2C_TIMEOUT,
        &i2c_context
    );

    return (status == CY_SCB_I2C_SUCCESS);
}


/* =========================================================
 * I2C READ
 * ========================================================= */

static bool IO_Expander_Read(
    uint8_t reg,
    uint8_t *data)
{
    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        IO_EXPANDER_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        I2C_TIMEOUT,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return false;
    }

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        reg,
        I2C_TIMEOUT,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            I2C_TIMEOUT,
            &i2c_context
        );

        return false;
    }

    status = Cy_SCB_I2C_MasterSendReStart(
        I2C_PHR_HW,
        IO_EXPANDER_ADDR,
        CY_SCB_I2C_READ_XFER,
        I2C_TIMEOUT,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            I2C_TIMEOUT,
            &i2c_context
        );

        return false;
    }

    status = Cy_SCB_I2C_MasterReadByte(
        I2C_PHR_HW,
        CY_SCB_I2C_NAK,
        data,
        I2C_TIMEOUT,
        &i2c_context
    );

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        I2C_TIMEOUT,
        &i2c_context
    );

    return (status == CY_SCB_I2C_SUCCESS);
}


/* =========================================================
 * SELECT PORT
 * ========================================================= */

static bool IO_Expander_SelectPort(
    uint8_t port)
{
    return IO_Expander_Write(
        REG_PORT_SELECT,
        port
    );
}


/* =========================================================
 * INITIALIZE CY8C9520A
 * ========================================================= */

static bool IO_Expander_Init(void)
{
    /* -----------------------------------------------------
     * PORT 0
     *
     * P0.0 - P0.7 = LED outputs
     * ----------------------------------------------------- */

    if (!IO_Expander_SelectPort(0))
    {
        return false;
    }

    /* All outputs */

    if (!IO_Expander_Write(
            REG_DIRECTION,
            0x00))
    {
        return false;
    }

    /* Disable pull-ups */

    if (!IO_Expander_Write(
            REG_DRIVE_PULLUP,
            0x00))
    {
        return false;
    }

    /* Strong drive */

    if (!IO_Expander_Write(
            REG_DRIVE_STRONG,
            0xFF))
    {
        return false;
    }

    /* No inversion */

    if (!IO_Expander_Write(
            REG_INVERSION,
            0x00))
    {
        return false;
    }

    /* All LEDs OFF */

    if (!IO_Expander_Write(
            REG_OUTPUT_PORT0,
            0x00))
    {
        return false;
    }


    /* -----------------------------------------------------
     * PORT 1
     *
     * P1.0 = SW1
     * P1.1 = SW2
     * P1.2 = SW3
     * P1.3 = SW4
     * ----------------------------------------------------- */

    if (!IO_Expander_SelectPort(1))
    {
        return false;
    }

    /* P1.0-P1.3 inputs */

    if (!IO_Expander_Write(
            REG_DIRECTION,
            0x0F))
    {
        return false;
    }

    /* Enable pull-ups */

    if (!IO_Expander_Write(
            REG_DRIVE_PULLUP,
            0xFF))
    {
        return false;
    }

    /* No inversion */

    if (!IO_Expander_Write(
            REG_INVERSION,
            0x00))
    {
        return false;
    }

    return true;
}


/* =========================================================
 * LED CONTROL
 * ========================================================= */

/*
 * Port 0 mapping:
 *
 * Bit 0 -> D2
 * Bit 1 -> D1
 * Bit 2 -> D4
 * Bit 3 -> D8
 * Bit 4 -> D5
 * Bit 5 -> D6
 * Bit 6 -> D3
 * Bit 7 -> D7
 */


/* All LEDs OFF */

static void LEDs_Off(void)
{
    IO_Expander_SelectPort(0);

    IO_Expander_Write(
        REG_OUTPUT_PORT0,
        0x00
    );
}


/* All LEDs ON */

static void LEDs_On(void)
{
    IO_Expander_SelectPort(0);

    IO_Expander_Write(
        REG_OUTPUT_PORT0,
        0xFF
    );
}


/* One LED ON */

static void LED_On(
    uint8_t led)
{
    IO_Expander_SelectPort(0);

    IO_Expander_Write(
        REG_OUTPUT_PORT0,
        (1 << led)
    );
}


/* =========================================================
 * READ SWITCHES
 * ========================================================= */

static uint8_t Read_Switches(void)
{
    uint8_t input;

    IO_Expander_SelectPort(1);

    if (!IO_Expander_Read(
            REG_INPUT_PORT1,
            &input))
    {
        return 0xFF;
    }

    return input;
}


/* =========================================================
 * APPLICATION INIT
 * ========================================================= */

void io_expander_app_init(void)
{
    io_expander_initialized = false;

    if (!IO_Expander_Init())
    {
        return;
    }

    io_expander_initialized = true;

    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: IO EXPANDER\r\n"
    );
}


/* =========================================================
 * APPLICATION RUN
 * ========================================================= */

void io_expander_app_run(void)
{
    uint8_t switches;

    if (!io_expander_initialized)
    {
        return;
    }

    switches = Read_Switches();


    /* =====================================================
     * SWITCH 1
     *
     * D2 -> D1 -> D4 -> D8
     * -> D5 -> D6 -> D3 -> D7
     * ===================================================== */

    if ((switches & 0x01) == 0)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            LED_On(i);

            Cy_SysLib_Delay(150);

            switches = Read_Switches();

            if ((switches & 0x01) != 0)
            {
                break;
            }
        }
    }


    /* =====================================================
     * SWITCH 2
     *
     * ALL LEDs ON
     * ===================================================== */

    else if ((switches & 0x02) == 0)
    {
        LEDs_On();

        Cy_SysLib_Delay(50);
    }


    /* =====================================================
     * SWITCH 3
     *
     * ALL LEDs BLINK
     * ===================================================== */

    else if ((switches & 0x04) == 0)
    {
        LEDs_On();

        Cy_SysLib_Delay(500);

        LEDs_Off();

        Cy_SysLib_Delay(500);
    }


    /* =====================================================
     * SWITCH 4
     *
     * D7 -> D3 -> D6 -> D5
     * -> D8 -> D4 -> D1 -> D2
     * ===================================================== */

    else if ((switches & 0x08) == 0)
    {
        for (int8_t i = 7; i >= 0; i--)
        {
            LED_On((uint8_t)i);

            Cy_SysLib_Delay(150);

            switches = Read_Switches();

            if ((switches & 0x08) != 0)
            {
                break;
            }
        }
    }


    /* =====================================================
     * NO SWITCH PRESSED
     * ===================================================== */

    else
    {
        LEDs_Off();

        Cy_SysLib_Delay(20);
    }
}


/* =========================================================
 * APPLICATION STOP
 * ========================================================= */

void io_expander_app_stop(void)
{
    LEDs_Off();

    io_expander_initialized = false;
}
#include "m95m04.h"
#include "cycfg_peripherals.h"
#include <string.h>

/* ---------------------------------------------------------
 * EEPROM CS control
 * EEPROM CS = P1[3]
 * --------------------------------------------------------- */

static void eeprom_cs_low(void)
{
    Cy_GPIO_Clr(EEPROM_CS_PORT, EEPROM_CS_PIN);
}

static void eeprom_cs_high(void)
{
    /*
     * Wait until the SPI transmission is completely finished
     * before releasing CS.
     */
    uint32_t timeout = 100000U;

    while (!Cy_SCB_SPI_IsTxComplete(SCB0) && timeout--)
    {
        ;
    }

    Cy_GPIO_Set(EEPROM_CS_PORT, EEPROM_CS_PIN);
}


/* ---------------------------------------------------------
 * Send one byte and receive one byte
 * --------------------------------------------------------- */

static uint8_t spi_transfer(uint8_t tx_data)
{
    /*
     * Make sure there is no old RX data.
     */
    Cy_SCB_SPI_ClearRxFifo(SCB0);

    /*
     * Put byte into TX FIFO.
     */
    Cy_SCB_SPI_Write(SCB0, tx_data);

    /*
     * Wait for received byte.
     */
    uint32_t timeout = 100000U;

    while (Cy_SCB_GetNumInRxFifo(SCB0) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0xFFU;
        }
    }

    return (uint8_t)Cy_SCB_SPI_Read(SCB0);
}


/* ---------------------------------------------------------
 * Initialize EEPROM CS
 * --------------------------------------------------------- */

void m95m04_init(void)
{
    /*
     * P1[3] is EEPROM CS.
     *
     * CS must be HIGH when EEPROM is idle.
     */
    Cy_GPIO_Pin_FastInit(
        EEPROM_CS_PORT,
        EEPROM_CS_PIN,
        CY_GPIO_DM_STRONG,
        1U,
        HSIOM_SEL_GPIO
    );

    Cy_GPIO_Set(
        EEPROM_CS_PORT,
        EEPROM_CS_PIN
    );
}


/* ---------------------------------------------------------
 * Read Status Register
 *
 * Command:
 *     CS LOW
 *     05h
 *     FFh
 *     CS HIGH
 * --------------------------------------------------------- */

uint8_t m95m04_read_status(void)
{
    uint8_t status;

    eeprom_cs_low();

    spi_transfer(M95M04_CMD_RDSR);

    status = spi_transfer(0xFFU);

    eeprom_cs_high();

    return status;
}


/* ---------------------------------------------------------
 * Write Enable
 *
 * Command:
 *     CS LOW
 *     06h
 *     CS HIGH
 * --------------------------------------------------------- */

bool m95m04_write_enable(void)
{
    eeprom_cs_low();

    spi_transfer(M95M04_CMD_WREN);

    eeprom_cs_high();

    /*
     * Give EEPROM a small amount of time before
     * reading the status register.
     */
    Cy_SysLib_DelayUs(10U);

    /*
     * WEL = bit 1
     */
    if ((m95m04_read_status() & 0x02U) != 0U)
    {
        return true;
    }

    return false;
}


/* ---------------------------------------------------------
 * Wait until EEPROM is no longer busy
 * --------------------------------------------------------- */

static bool m95m04_wait_ready(void)
{
    uint32_t timeout = 100000U;

    while (m95m04_read_status() & 0x01U)
    {
        if (--timeout == 0U)
        {
            return false;
        }
    }

    return true;
}


/* ---------------------------------------------------------
 * Write one byte
 *
 * WRITE:
 *     02h
 *     Address[23:16]
 *     Address[15:8]
 *     Address[7:0]
 *     DATA
 * --------------------------------------------------------- */

bool m95m04_write_byte(uint32_t address, uint8_t data)
{
    if (!m95m04_wait_ready())
    {
        return false;
    }

    /*
     * Set WEL.
     */
    if (!m95m04_write_enable())
    {
        return false;
    }

    /*
     * Start WRITE command.
     */
    eeprom_cs_low();

    spi_transfer(M95M04_CMD_WRITE);

    spi_transfer((uint8_t)(address >> 16));
    spi_transfer((uint8_t)(address >> 8));
    spi_transfer((uint8_t)(address));

    spi_transfer(data);

    eeprom_cs_high();

    /*
     * Wait for internal EEPROM write cycle.
     */
    if (!m95m04_wait_ready())
    {
        return false;
    }

    return true;
}


/* ---------------------------------------------------------
 * Read one byte
 *
 * READ:
 *     03h
 *     Address[23:16]
 *     Address[15:8]
 *     Address[7:0]
 *     FFh
 * --------------------------------------------------------- */

bool m95m04_read_byte(uint32_t address, uint8_t *data)
{
    if (data == NULL)
    {
        return false;
    }

    if (!m95m04_wait_ready())
    {
        return false;
    }

    eeprom_cs_low();

    spi_transfer(M95M04_CMD_READ);

    spi_transfer((uint8_t)(address >> 16));
    spi_transfer((uint8_t)(address >> 8));
    spi_transfer((uint8_t)(address));

    *data = spi_transfer(0xFFU);

    eeprom_cs_high();

    return true;
}


/* ---------------------------------------------------------
 * EEPROM test
 * --------------------------------------------------------- */

bool m95m04_test(void)
{
    uint8_t status;
    uint8_t read_data;

    /*
     * -----------------------------------------------------
     * TEST 1: Initial status
     * -----------------------------------------------------
     */

    Cy_SCB_UART_PutString(
        SCB3,
        "\r\n[M95M04] Reading initial status...\r\n"
    );

    status = m95m04_read_status();

    /*
     * Print status manually in hexadecimal.
     */
    const char hex[] = "0123456789ABCDEF";
    char msg[40];

    msg[0] = 'S';
    msg[1] = 't';
    msg[2] = 'a';
    msg[3] = 't';
    msg[4] = 'u';
    msg[5] = 's';
    msg[6] = ' ';
    msg[7] = '=';
    msg[8] = ' ';
    msg[9] = '0';
    msg[10] = 'x';
    msg[11] = hex[(status >> 4) & 0x0F];
    msg[12] = hex[status & 0x0F];
    msg[13] = '\r';
    msg[14] = '\n';
    msg[15] = '\0';

    Cy_SCB_UART_PutString(SCB3, msg);


    /*
     * -----------------------------------------------------
     * TEST 2: WREN
     * -----------------------------------------------------
     */

    Cy_SCB_UART_PutString(
        SCB3,
        "[M95M04] Sending WREN...\r\n"
    );

    if (!m95m04_write_enable())
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[M95M04] ERROR: WEL did not become 1!\r\n"
        );

        return false;
    }

    Cy_SCB_UART_PutString(
        SCB3,
        "[M95M04] WREN successful. WEL = 1\r\n"
    );


    /*
     * -----------------------------------------------------
     * TEST 3: Write 0x5A
     * -----------------------------------------------------
     */

    Cy_SCB_UART_PutString(
        SCB3,
        "[M95M04] Writing 0x5A to address 0x000000...\r\n"
    );

    if (!m95m04_write_byte(0x000000U, 0x5AU))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[M95M04] ERROR: Write failed!\r\n"
        );

        return false;
    }

    Cy_SCB_UART_PutString(
        SCB3,
        "[M95M04] Write successful.\r\n"
    );


    /*
     * -----------------------------------------------------
     * TEST 4: Read back
     * -----------------------------------------------------
     */

    Cy_SCB_UART_PutString(
        SCB3,
        "[M95M04] Reading address 0x000000...\r\n"
    );

    if (!m95m04_read_byte(0x000000U, &read_data))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[M95M04] ERROR: Read failed!\r\n"
        );

        return false;
    }


    /*
     * Print received byte.
     */

    msg[0] = 'R';
    msg[1] = 'e';
    msg[2] = 'a';
    msg[3] = 'd';
    msg[4] = ' ';
    msg[5] = '=';
    msg[6] = ' ';
    msg[7] = '0';
    msg[8] = 'x';
    msg[9] = hex[(read_data >> 4) & 0x0F];
    msg[10] = hex[read_data & 0x0F];
    msg[11] = '\r';
    msg[12] = '\n';
    msg[13] = '\0';

    Cy_SCB_UART_PutString(SCB3, msg);


    /*
     * -----------------------------------------------------
     * TEST 5: Verify
     * -----------------------------------------------------
     */

    if (read_data == 0x5AU)
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[M95M04] EEPROM TEST PASSED!\r\n"
        );

        return true;
    }

    Cy_SCB_UART_PutString(
        SCB3,
        "[M95M04] ERROR: Readback mismatch!\r\n"
    );

    return false;
}
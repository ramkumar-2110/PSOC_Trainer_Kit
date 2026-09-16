#include "tpm_spi.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "cybsp.h"
#include "cycfg_peripherals.h"


#define TPM_SPI_ADDR_PREFIX        0xD4U

#define TPM_DID_VID_ADDR           0x0F00U
#define TPM_RID_ADDR               0x0F04U

#define TPM_EXPECTED_VENDOR_ID     0x15D1U
#define TPM_EXPECTED_DEVICE_ID     0x001BU


/* SCB0 SPI context is created in main.c */
extern cy_stc_scb_spi_context_t spi_context;


/* ============================================================
 * Debug helper
 * ============================================================ */
static void tpm_print_byte(
    const char *name,
    uint8_t value)
{
    char msg[40];

    sprintf(
        msg,
        "%s : 0x%02X\r\n",
        name,
        value
    );

    Cy_SCB_UART_PutString(
        SCB3,
        msg
    );
}


/* ============================================================
 * Flush RX FIFO
 * ============================================================ */
static void tpm_flush_rx(void)
{
    while (Cy_SCB_SPI_GetNumInRxFifo(SCB0) > 0U)
    {
        (void)Cy_SCB_SPI_Read(SCB0);
    }
}


/* ============================================================
 * SPI transfer
 *
 * CS is controlled by SCB0 hardware SS2.
 *
 * P1_5 = SECURE_CS
 *      = SCB0 SPI.select[2]
 * ============================================================ */
static bool tpm_spi_transfer(
    const uint8_t *tx,
    uint8_t *rx,
    uint32_t length)
{
    uint32_t rx_count;


    if ((tx == NULL) ||
        (rx == NULL) ||
        (length == 0U))
    {
        return false;
    }


    /* Clear stale RX data */
    tpm_flush_rx();


    /*
     * Send all bytes.
     *
     * enableTransferSeperation = false
     *
     * Therefore CS remains active for the complete
     * SPI transfer.
     */
    Cy_SCB_SPI_WriteArrayBlocking(
        SCB0,
        (void *)tx,
        length
    );


    /*
     * Wait until the last byte has physically
     * left the SPI shifter.
     */
    while (!Cy_SCB_SPI_IsTxComplete(SCB0))
    {
    }


    /*
     * Wait until all received bytes are available.
     *
     * This is important for the TPM diagnostic.
     */
    do
    {
        rx_count =
            Cy_SCB_SPI_GetNumInRxFifo(SCB0);

    } while (rx_count < length);


    /*
     * Read exactly 'length' bytes.
     */
    for (uint32_t i = 0U; i < length; i++)
    {
        rx[i] =
            (uint8_t)Cy_SCB_SPI_Read(SCB0);
    }


    return true;
}


/* ============================================================
 * Initialize TPM SPI
 * ============================================================ */
bool tpm_spi_init(void)
{
    /*
     * SCB0 itself is initialized in main.c.
     */
    if ((SCB0->CTRL & SCB_CTRL_ENABLED_Msk) == 0UL)
    {
        return false;
    }


    return true;
}


/* ============================================================
 * Read TPM DID/VID
 *
 * SLB9670:
 *
 *     TX:
 *
 *     83 D4 0F 00 00 00 00 00
 *
 *     RX:
 *
 *     00 00 00 01 D1 15 1B 00
 *
 * Expected:
 *
 *     Vendor ID = 0x15D1
 *     Device ID = 0x001B
 * ============================================================ */
bool tpm_read_did_vid(uint32_t *did_vid)
{
    uint8_t tx[8];
    uint8_t rx[8];

    uint32_t value;

    char msg[50];


    if (did_vid == NULL)
    {
        return false;
    }


    /* --------------------------------------------------------
     * Build TPM register read
     * -------------------------------------------------------- */
    tx[0] = 0x83U;
    tx[1] = 0xD4U;
    tx[2] = 0x0FU;
    tx[3] = 0x00U;

    tx[4] = 0x00U;
    tx[5] = 0x00U;
    tx[6] = 0x00U;
    tx[7] = 0x00U;


    for (uint32_t i = 0U; i < 8U; i++)
    {
        rx[i] = 0U;
    }


    /* --------------------------------------------------------
     * Show TX bytes
     * -------------------------------------------------------- */
    Cy_SCB_UART_PutString(
        SCB3,
        "TPM TX: 83 D4 0F 00 00 00 00 00\r\n"
    );


    /* --------------------------------------------------------
     * Perform transaction
     * -------------------------------------------------------- */
    if (!tpm_spi_transfer(
            tx,
            rx,
            8U))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "TPM SPI TRANSFER FAILED\r\n"
        );

        return false;
    }


    /* --------------------------------------------------------
     * Print RAW RX bytes
     * -------------------------------------------------------- */
    sprintf(
        msg,
        "RX[0] = 0x%02X\r\n",
        rx[0]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    sprintf(
        msg,
        "RX[1] = 0x%02X\r\n",
        rx[1]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    sprintf(
        msg,
        "RX[2] = 0x%02X\r\n",
        rx[2]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    sprintf(
        msg,
        "RX[3] = 0x%02X\r\n",
        rx[3]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    sprintf(
        msg,
        "RX[4] = 0x%02X\r\n",
        rx[4]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    sprintf(
        msg,
        "RX[5] = 0x%02X\r\n",
        rx[5]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    sprintf(
        msg,
        "RX[6] = 0x%02X\r\n",
        rx[6]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    sprintf(
        msg,
        "RX[7] = 0x%02X\r\n",
        rx[7]
    );
    Cy_SCB_UART_PutString(SCB3, msg);


    /* --------------------------------------------------------
     * Expected response:
     *
     * 00 00 00 01 D1 15 1B 00
     * -------------------------------------------------------- */
    if ((rx[0] != 0x00U) ||
        (rx[1] != 0x00U) ||
        (rx[2] != 0x00U) ||
        (rx[3] != 0x01U))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "TPM: INVALID SPI RESPONSE\r\n"
        );

        return false;
    }


    /* --------------------------------------------------------
     * DID/VID is LSB-first
     *
     * D1 15 1B 00
     *
     * -> 0x001B15D1
     * -------------------------------------------------------- */
    value =
          ((uint32_t)rx[4])
        | ((uint32_t)rx[5] << 8U)
        | ((uint32_t)rx[6] << 16U)
        | ((uint32_t)rx[7] << 24U);


    *did_vid = value;


    return true;
}


/* ============================================================
 * Read RID
 *
 * Request:
 *
 *     80 D4 0F 04
 *
 * Then one dummy byte.
 * ============================================================ */
bool tpm_read_rid(uint8_t *rid)
{
    uint8_t tx[5];
    uint8_t rx[5];


    if (rid == NULL)
    {
        return false;
    }


    tx[0] = 0x80U;
    tx[1] = 0xD4U;
    tx[2] = 0x0FU;
    tx[3] = 0x04U;
    tx[4] = 0x00U;


    for (uint32_t i = 0U; i < 5U; i++)
    {
        rx[i] = 0U;
    }


    if (!tpm_spi_transfer(
            tx,
            rx,
            5U))
    {
        return false;
    }


    *rid = rx[4];


    return true;
}
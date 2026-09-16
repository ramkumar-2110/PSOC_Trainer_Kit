#include "secure_app.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "cybsp.h"

#include "tpm_spi.h"


static bool secure_initialized = false;


void secure_app_init(void)
{
    uint32_t did_vid = 0U;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t rid = 0U;

    char message[80];


    secure_initialized = false;


    Cy_SCB_UART_PutString(
        SCB3,
        "\r\n"
        "================================\r\n"
        "       TPM DEVICE INFORMATION\r\n"
        "================================\r\n"
    );


    /* --------------------------------------------------------
     * Check SPI
     * -------------------------------------------------------- */
    if (!tpm_spi_init())
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "TPM SPI : INIT FAILED\r\n"
        );

        return;
    }


    Cy_SCB_UART_PutString(
        SCB3,
        "TPM SPI : READY\r\n"
    );


    /* --------------------------------------------------------
     * DID/VID
     * -------------------------------------------------------- */
    if (!tpm_read_did_vid(&did_vid))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "TPM DID/VID : READ FAILED\r\n"
        );

        Cy_SCB_UART_PutString(
            SCB3,
            "TPM : NOT DETECTED\r\n"
        );

        return;
    }


    vendor_id =
        (uint16_t)(did_vid & 0xFFFFU);

    device_id =
        (uint16_t)((did_vid >> 16U) & 0xFFFFU);


    sprintf(
        message,
        "DID/VID : 0x%08lX\r\n",
        (unsigned long)did_vid
    );

    Cy_SCB_UART_PutString(
        SCB3,
        message
    );


    sprintf(
        message,
        "Vendor ID : 0x%04X\r\n",
        vendor_id
    );

    Cy_SCB_UART_PutString(
        SCB3,
        message
    );


    sprintf(
        message,
        "Device ID : 0x%04X\r\n",
        device_id
    );

    Cy_SCB_UART_PutString(
        SCB3,
        message
    );


    /* --------------------------------------------------------
     * SLB9670 expected identification
     * -------------------------------------------------------- */
    if ((vendor_id == 0x15D1U) &&
        (device_id == 0x001BU))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "TPM : SLB9670 DETECTED\r\n"
        );
    }
    else
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "TPM : UNKNOWN DEVICE\r\n"
        );

        return;
    }


    /* --------------------------------------------------------
     * RID
     * -------------------------------------------------------- */
    if (tpm_read_rid(&rid))
    {
        sprintf(
            message,
            "Revision ID : 0x%02X\r\n",
            rid
        );

        Cy_SCB_UART_PutString(
            SCB3,
            message
        );
    }


    Cy_SCB_UART_PutString(
        SCB3,
        "TPM SPI communication : OK\r\n"
    );


    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: SECURE\r\n"
    );


    secure_initialized = true;
}


void secure_app_run(void)
{
    (void)secure_initialized;
}


void secure_app_stop(void)
{
    secure_initialized = false;
}
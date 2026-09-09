#include "sd_card.h"
#include "cycfg_peripherals.h"
#include "cycfg_pins.h"

#define CMD0       0U
#define CMD8       8U
#define CMD9       9U
#define CMD12      12U
#define CMD16      16U
#define CMD17      17U
#define CMD24      24U
#define CMD55      55U
#define CMD58      58U
#define ACMD41     41U

static cy_stc_scb_spi_context_t sd_spi_context;

static bool sd_ready = false;
static uint8_t sd_type = SD_CARD_TYPE_NONE;


/* =========================================================
 * CS CONTROL
 * ========================================================= */

static void sd_cs_low(void)
{
    Cy_GPIO_Clr(SD_CS_PORT, SD_CS_PIN);
}


static void sd_cs_high(void)
{
    uint32_t timeout = 100000U;

    while (!Cy_SCB_SPI_IsTxComplete(SCB0))
    {
        if (--timeout == 0U)
            break;
    }

    Cy_GPIO_Set(SD_CS_PORT, SD_CS_PIN);
}


/* =========================================================
 * SPI TRANSFER
 * ========================================================= */

static uint8_t sd_spi_transfer(uint8_t tx)
{
    uint32_t timeout = 100000U;

    Cy_SCB_SPI_Write(SCB0, tx);

    while (Cy_SCB_SPI_GetNumInRxFifo(SCB0) == 0U)
    {
        if (--timeout == 0U)
            return 0xFFU;
    }

    return (uint8_t)Cy_SCB_SPI_Read(SCB0);
}


/* =========================================================
 * SPI CLOCK
 *
 * Device Configurator assigned SCB0 to divider #6.
 *
 * 48 MHz / 120 = 400 kHz
 * 48 MHz / 24  = 2 MHz
 * ========================================================= */

static void sd_set_clock(uint32_t divider)
{
    Cy_SysClk_PeriphDisableDivider(
        CY_SYSCLK_DIV_16_BIT,
        6U
    );

    Cy_SysClk_PeriphSetDivider(
        CY_SYSCLK_DIV_16_BIT,
        6U,
        divider
    );

    Cy_SysClk_PeriphEnableDivider(
        CY_SYSCLK_DIV_16_BIT,
        6U
    );
}


/* =========================================================
 * SPI INITIALIZATION
 * ========================================================= */

static bool sd_spi_init(void)
{
    /*
     * Use the Device Configurator generated pin routing.
     *
     * MOSI = P1[0]
     * MISO = P1[1]
     * SCK  = P1[2]
     */

    Cy_GPIO_Pin_FastInit(
        SD_MOSI_PORT,
        SD_MOSI_PIN,
        CY_GPIO_DM_STRONG,
        1U,
        P1_0_SCB0_SPI_MOSI
    );

    Cy_GPIO_Pin_FastInit(
        SD_MISO_PORT,
        SD_MISO_PIN,
        CY_GPIO_DM_HIGHZ,
        1U,
        P1_1_SCB0_SPI_MISO
    );

    Cy_GPIO_Pin_FastInit(
        SD_SCK_PORT,
        SD_SCK_PIN,
        CY_GPIO_DM_STRONG,
        1U,
        P1_2_SCB0_SPI_CLK
    );


    /*
     * P1[4] is NOT hardware SS.
     * We control SD CS manually.
     */

    Cy_GPIO_Pin_FastInit(
        SD_CS_PORT,
        SD_CS_PIN,
        CY_GPIO_DM_STRONG,
        1U,
        HSIOM_SEL_GPIO
    );

    Cy_GPIO_Set(
        SD_CS_PORT,
        SD_CS_PIN
    );


    /*
     * Device Configurator generated SCB0 configuration.
     */

    if (Cy_SCB_SPI_Init(
            SCB0,
            &scb_0_config,
            &sd_spi_context) != CY_SCB_SPI_SUCCESS)
    {
        return false;
    }

    Cy_SCB_SPI_Enable(SCB0);

    /*
     * SD initialization clock = 400 kHz.
     */
    sd_set_clock(119U);

    return true;
}


/* =========================================================
 * SEND SD COMMAND
 * ========================================================= */

static uint8_t sd_send_cmd(
    uint8_t cmd,
    uint32_t arg)
{
    uint8_t response;
    uint16_t timeout;

    /*
     * Start a fresh command transaction.
     */
    sd_cs_high();
    sd_spi_transfer(0xFFU);

    sd_cs_low();

    /*
     * Command packet.
     */
    sd_spi_transfer(0x40U | cmd);

    sd_spi_transfer((uint8_t)(arg >> 24));
    sd_spi_transfer((uint8_t)(arg >> 16));
    sd_spi_transfer((uint8_t)(arg >> 8));
    sd_spi_transfer((uint8_t)arg);

    /*
     * CRC:
     *
     * CMD0 = 0x95
     * CMD8 = 0x87
     * Other commands can use dummy CRC
     * after entering SPI mode.
     */
    uint8_t crc = 0x01U;

    if (cmd == CMD0)
        crc = 0x95U;

    if (cmd == CMD8)
        crc = 0x87U;

    sd_spi_transfer(crc);


    /*
     * Wait for R1.
     */
    timeout = 1000U;

    do
    {
        response = sd_spi_transfer(0xFFU);

        if ((response & 0x80U) == 0U)
            return response;

    } while (--timeout);

    return 0xFFU;
}


/* =========================================================
 * CARD INITIALIZATION
 * ========================================================= */

bool sd_init(void)
{
    uint8_t r1;
    uint8_t ocr[4];

    sd_ready = false;
    sd_type = SD_CARD_TYPE_NONE;


    if (!sd_spi_init())
        return false;


    /*
     * At least 74 clock cycles with CS HIGH.
     */
    sd_cs_high();

    for (uint8_t i = 0; i < 15U; i++)
        sd_spi_transfer(0xFFU);


    /*
     * CMD0
     */
    for (uint16_t retry = 0; retry < 1000U; retry++)
    {
        r1 = sd_send_cmd(CMD0, 0U);

        if (r1 == 0x01U)
            break;

        sd_cs_high();
        Cy_SysLib_Delay(1U);
    }

    if (r1 != 0x01U)
    {
        sd_cs_high();
        return false;
    }


    /*
     * CMD8
     */
    r1 = sd_send_cmd(CMD8, 0x000001AAUL);

    if (r1 == 0x01U)
    {
        /*
         * Read R7 response.
         */
        for (uint8_t i = 0; i < 4U; i++)
            ocr[i] = sd_spi_transfer(0xFFU);


        /*
         * Check voltage pattern.
         */
        if (ocr[2] != 0x01U || ocr[3] != 0xAAU)
        {
            sd_cs_high();
            return false;
        }


        /*
         * ACMD41
         */
        uint32_t timeout = 2000U;

        do
        {
            sd_cs_high();
            sd_spi_transfer(0xFFU);

            sd_send_cmd(CMD55, 0U);

            sd_cs_high();
            sd_spi_transfer(0xFFU);

            r1 = sd_send_cmd(
                ACMD41,
                0x40000000UL
            );

            sd_cs_high();
            sd_spi_transfer(0xFFU);

            if (r1 == 0x00U)
                break;

            Cy_SysLib_Delay(1U);

        } while (--timeout);


        if (r1 != 0x00U)
        {
            return false;
        }


        /*
         * CMD58 - read OCR.
         */
        r1 = sd_send_cmd(CMD58, 0U);

        if (r1 != 0x00U)
        {
            sd_cs_high();
            return false;
        }

        for (uint8_t i = 0; i < 4U; i++)
            ocr[i] = sd_spi_transfer(0xFFU);


        /*
         * CCS = 1 → SDHC/SDXC.
         */
        if (ocr[0] & 0x40U)
            sd_type = SD_CARD_TYPE_SDHC;
        else
            sd_type = SD_CARD_TYPE_SD2;
    }
    else
    {
        /*
         * SD Version 1.
         */
        sd_cs_high();
        sd_spi_transfer(0xFFU);

        sd_send_cmd(CMD55, 0U);

        sd_cs_high();
        sd_spi_transfer(0xFFU);

        r1 = sd_send_cmd(ACMD41, 0U);

        uint32_t timeout = 2000U;

        while (r1 != 0x00U && timeout--)
        {
            sd_cs_high();
            sd_spi_transfer(0xFFU);

            sd_send_cmd(CMD55, 0U);

            sd_cs_high();
            sd_spi_transfer(0xFFU);

            r1 = sd_send_cmd(ACMD41, 0U);

            Cy_SysLib_Delay(1U);
        }

        if (r1 == 0x00U)
            sd_type = SD_CARD_TYPE_SD1;
    }


    sd_cs_high();
    sd_spi_transfer(0xFFU);


    if (sd_type == SD_CARD_TYPE_NONE)
        return false;


    /*
     * SDSC cards use byte addressing.
     */
    if (sd_type != SD_CARD_TYPE_SDHC)
    {
        r1 = sd_send_cmd(CMD16, 512U);

        sd_cs_high();
        sd_spi_transfer(0xFFU);

        if (r1 != 0x00U)
            return false;
    }


    /*
     * Normal operating SPI speed = 2 MHz.
     */
    sd_set_clock(23U);


    sd_ready = true;

    return true;
}


/* =========================================================
 * READ ONE 512-BYTE SECTOR
 * ========================================================= */

bool sd_read_sector(
    uint32_t sector,
    uint8_t *buffer)
{
    if (!sd_ready || buffer == NULL)
        return false;


    uint32_t address;

    if (sd_type == SD_CARD_TYPE_SDHC)
        address = sector;
    else
        address = sector * 512UL;


    /*
     * CMD17
     */
    if (sd_send_cmd(CMD17, address) != 0x00U)
    {
        sd_cs_high();
        sd_spi_transfer(0xFFU);
        return false;
    }


    /*
     * Wait for 0xFE data token.
     */
    uint32_t timeout = 100000U;
    uint8_t token;

    do
    {
        token = sd_spi_transfer(0xFFU);

        if (token == 0xFEU)
            break;

    } while (--timeout);


    if (token != 0xFEU)
    {
        sd_cs_high();
        sd_spi_transfer(0xFFU);
        return false;
    }


    /*
     * Read exactly 512 bytes.
     */
    for (uint16_t i = 0; i < 512U; i++)
    {
        buffer[i] = sd_spi_transfer(0xFFU);
    }


    /*
     * CRC.
     */
    sd_spi_transfer(0xFFU);
    sd_spi_transfer(0xFFU);


    sd_cs_high();
    sd_spi_transfer(0xFFU);

    return true;
}


/* =========================================================
 * WRITE ONE 512-BYTE SECTOR
 * ========================================================= */

bool sd_write_sector(
    uint32_t sector,
    const uint8_t *buffer)
{
    if (!sd_ready || buffer == NULL)
        return false;


    uint32_t address;

    if (sd_type == SD_CARD_TYPE_SDHC)
        address = sector;
    else
        address = sector * 512UL;


    /*
     * CMD24
     */
    if (sd_send_cmd(CMD24, address) != 0x00U)
    {
        sd_cs_high();
        sd_spi_transfer(0xFFU);
        return false;
    }


    /*
     * Data token.
     */
    sd_spi_transfer(0xFFU);
    sd_spi_transfer(0xFEU);


    /*
     * 512 bytes.
     */
    for (uint16_t i = 0; i < 512U; i++)
    {
        sd_spi_transfer(buffer[i]);
    }


    /*
     * Dummy CRC.
     */
    sd_spi_transfer(0xFFU);
    sd_spi_transfer(0xFFU);


    /*
     * Data response.
     */
    uint8_t response;
    uint32_t timeout = 100000U;

    do
    {
        response = sd_spi_transfer(0xFFU);

        if (response != 0xFFU)
            break;

    } while (--timeout);


    if ((response & 0x1FU) != 0x05U)
    {
        sd_cs_high();
        sd_spi_transfer(0xFFU);
        return false;
    }


    /*
     * Wait until card finishes internal write.
     */
    timeout = 1000000UL;

    while (sd_spi_transfer(0xFFU) == 0x00U)
    {
        if (--timeout == 0U)
        {
            sd_cs_high();
            sd_spi_transfer(0xFFU);
            return false;
        }
    }


    sd_cs_high();
    sd_spi_transfer(0xFFU);

    return true;
}


bool sd_is_ready(void)
{
    return sd_ready;
}


uint8_t sd_get_type(void)
{
    return sd_type;
}
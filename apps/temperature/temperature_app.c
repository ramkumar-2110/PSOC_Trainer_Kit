#include "temperature_app.h"

#include "cy_pdl.h"
#include "cycfg.h"
#include "cycfg_peripherals.h"

#include <stdint.h>
#include <stdio.h>

/* =========================================================
 * SHARED I2C CONTEXT
 * ========================================================= */

extern cy_stc_scb_i2c_context_t i2c_context;


/* =========================================================
 * TMP102
 * ========================================================= */

#define TMP102_ADDR        0x48
#define TMP102_TEMP_REG    0x00


/* =========================================================
 * OLED SSD1306
 * ========================================================= */

#define OLED_ADDR          0x3C
#define OLED_WIDTH         128
#define OLED_HEIGHT        64

static uint8_t oled_buffer[
    OLED_WIDTH * OLED_HEIGHT / 8
];


/* =========================================================
 * APPLICATION STATE
 * ========================================================= */

static uint32_t temperature_counter = 0;
static bool temperature_app_active = false;


/* =========================================================
 * TMP102 READ TEMPERATURE
 * ========================================================= */

static int16_t TMP102_ReadTemperature(void)
{
    uint8_t msb;
    uint8_t lsb;

    uint16_t raw;

    int16_t temp_raw;

    cy_en_scb_i2c_status_t status;


    /* START + WRITE */

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        TMP102_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return -9999;
    }


    /* Select temperature register */

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        TMP102_TEMP_REG,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            1000,
            &i2c_context
        );

        return -9999;
    }


    /* RESTART + READ */

    status = Cy_SCB_I2C_MasterSendReStart(
        I2C_PHR_HW,
        TMP102_ADDR,
        CY_SCB_I2C_READ_XFER,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            1000,
            &i2c_context
        );

        return -9999;
    }


    /* Read MSB */

    status = Cy_SCB_I2C_MasterReadByte(
        I2C_PHR_HW,
        CY_SCB_I2C_ACK,
        &msb,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            1000,
            &i2c_context
        );

        return -9999;
    }


    /* Read LSB */

    status = Cy_SCB_I2C_MasterReadByte(
        I2C_PHR_HW,
        CY_SCB_I2C_NAK,
        &lsb,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            1000,
            &i2c_context
        );

        return -9999;
    }


    /* STOP */

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );


    /* Convert TMP102 data */

    raw =
        ((uint16_t)msb << 8) | lsb;

    temp_raw =
        (int16_t)raw;

    temp_raw >>= 4;


    return temp_raw;
}


/* =========================================================
 * OLED WRITE BYTE
 * ========================================================= */

static cy_en_scb_i2c_status_t OLED_WriteByte(
    uint8_t data)
{
    return Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        data,
        1000,
        &i2c_context
    );
}


/* =========================================================
 * OLED COMMAND
 * ========================================================= */

static bool OLED_Command(uint8_t command)
{
    cy_en_scb_i2c_status_t status;


    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        OLED_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return false;
    }


    /* Command control byte */

    status = OLED_WriteByte(0x00);

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            1000,
            &i2c_context
        );

        return false;
    }


    /* Command */

    status = OLED_WriteByte(command);

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            1000,
            &i2c_context
        );

        return false;
    }


    /* STOP */

    status = Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );


    return (
        status == CY_SCB_I2C_SUCCESS
    );
}


/* =========================================================
 * OLED SEND PAGE
 * ========================================================= */

static bool OLED_SendPage(uint8_t page)
{
    uint16_t i;

    cy_en_scb_i2c_status_t status;


    /* Page address */

    if (!OLED_Command(0xB0 + page))
    {
        return false;
    }


    /* Lower column */

    if (!OLED_Command(0x00))
    {
        return false;
    }


    /* Higher column */

    if (!OLED_Command(0x10))
    {
        return false;
    }


    /* Start data transfer */

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        OLED_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return false;
    }


    /* Data control byte */

    status = OLED_WriteByte(0x40);

    if (status != CY_SCB_I2C_SUCCESS)
    {
        Cy_SCB_I2C_MasterSendStop(
            I2C_PHR_HW,
            1000,
            &i2c_context
        );

        return false;
    }


    /* Send 128 bytes */

    for (i = 0; i < 128; i++)
    {
        status = OLED_WriteByte(
            oled_buffer[
                page * 128 + i
            ]
        );

        if (status != CY_SCB_I2C_SUCCESS)
        {
            Cy_SCB_I2C_MasterSendStop(
                I2C_PHR_HW,
                1000,
                &i2c_context
            );

            return false;
        }
    }


    /* STOP */

    status = Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );


    return (
        status == CY_SCB_I2C_SUCCESS
    );
}


/* =========================================================
 * OLED UPDATE
 * ========================================================= */

static bool OLED_Update(void)
{
    uint8_t page;

    for (page = 0; page < 8; page++)
    {
        if (!OLED_SendPage(page))
        {
            return false;
        }
    }

    return true;
}


/* =========================================================
 * OLED CLEAR
 * ========================================================= */

static void OLED_Clear(void)
{
    uint16_t i;

    for (i = 0; i < sizeof(oled_buffer); i++)
    {
        oled_buffer[i] = 0x00;
    }
}


/* =========================================================
 * OLED DRAW PIXEL
 * ========================================================= */

static void OLED_DrawPixel(
    uint8_t x,
    uint8_t y)
{
    if (x >= OLED_WIDTH ||
        y >= OLED_HEIGHT)
    {
        return;
    }

    oled_buffer[
        x + ((y / 8) * OLED_WIDTH)
    ] |=
        (1U << (y % 8));
}


/* =========================================================
 * FONT 0
 * ========================================================= */

static const uint8_t font_0[5] =
{
    0x3E, 0x51, 0x49, 0x45, 0x3E
};


/* =========================================================
 * FONT 1
 * ========================================================= */

static const uint8_t font_1[5] =
{
    0x00, 0x42, 0x7F, 0x40, 0x00
};


/* =========================================================
 * FONT 2
 * ========================================================= */

static const uint8_t font_2[5] =
{
    0x42, 0x61, 0x51, 0x49, 0x46
};


/* =========================================================
 * FONT 3
 * ========================================================= */

static const uint8_t font_3[5] =
{
    0x21, 0x41, 0x45, 0x4B, 0x31
};


/* =========================================================
 * FONT 4
 * ========================================================= */

static const uint8_t font_4[5] =
{
    0x18, 0x14, 0x12, 0x7F, 0x10
};


/* =========================================================
 * FONT 5
 * ========================================================= */

static const uint8_t font_5[5] =
{
    0x27, 0x45, 0x45, 0x45, 0x39
};


/* =========================================================
 * FONT 6
 * ========================================================= */

static const uint8_t font_6[5] =
{
    0x3C, 0x4A, 0x49, 0x49, 0x30
};


/* =========================================================
 * FONT 7
 * ========================================================= */

static const uint8_t font_7[5] =
{
    0x01, 0x71, 0x09, 0x05, 0x03
};


/* =========================================================
 * FONT 8
 * ========================================================= */

static const uint8_t font_8[5] =
{
    0x36, 0x49, 0x49, 0x49, 0x36
};


/* =========================================================
 * FONT 9
 * ========================================================= */

static const uint8_t font_9[5] =
{
    0x06, 0x49, 0x49, 0x29, 0x1E
};


/* =========================================================
 * FONT DECIMAL POINT
 * ========================================================= */

static const uint8_t font_dot[5] =
{
    0x00, 0x60, 0x60, 0x00, 0x00
};


/* =========================================================
 * FONT C
 * ========================================================= */

static const uint8_t font_C[5] =
{
    0x3E, 0x41, 0x41, 0x41, 0x22
};


/* =========================================================
 * GET FONT
 * ========================================================= */

static const uint8_t *OLED_GetFont(char c)
{
    switch (c)
    {
        case '0': return font_0;
        case '1': return font_1;
        case '2': return font_2;
        case '3': return font_3;
        case '4': return font_4;
        case '5': return font_5;
        case '6': return font_6;
        case '7': return font_7;
        case '8': return font_8;
        case '9': return font_9;
        case '.': return font_dot;
        case 'C': return font_C;

        default:
            return NULL;
    }
}


/* =========================================================
 * DRAW CHARACTER
 * ========================================================= */

static void OLED_DrawChar(
    uint8_t x,
    uint8_t y,
    char c,
    uint8_t scale)
{
    const uint8_t *bitmap;

    uint8_t column;
    uint8_t row;
    uint8_t dx;
    uint8_t dy;


    bitmap = OLED_GetFont(c);


    if (bitmap == NULL)
    {
        return;
    }


    for (column = 0;
         column < 5;
         column++)
    {
        for (row = 0;
             row < 7;
             row++)
        {
            if (bitmap[column] &
                (1U << row))
            {
                for (dx = 0;
                     dx < scale;
                     dx++)
                {
                    for (dy = 0;
                         dy < scale;
                         dy++)
                    {
                        OLED_DrawPixel(
                            x +
                            column * scale +
                            dx,

                            y +
                            row * scale +
                            dy
                        );
                    }
                }
            }
        }
    }
}


/* =========================================================
 * DRAW STRING
 * ========================================================= */

static void OLED_DrawString(
    uint8_t x,
    uint8_t y,
    const char *text,
    uint8_t scale)
{
    while (*text)
    {
        OLED_DrawChar(
            x,
            y,
            *text,
            scale
        );

        x += 6 * scale;

        text++;
    }
}


/* =========================================================
 * COMPACT FONT T
 * ========================================================= */

static const uint8_t font_T4[4] =
{
    0x01, 0x01, 0x7F, 0x01
};


/* =========================================================
 * COMPACT FONT E
 * ========================================================= */

static const uint8_t font_E4[4] =
{
    0x7F, 0x49, 0x49, 0x41
};


/* =========================================================
 * COMPACT FONT M
 * ========================================================= */

static const uint8_t font_M4[4] =
{
    0x7F, 0x06, 0x06, 0x7F
};


/* =========================================================
 * COMPACT FONT P
 * ========================================================= */

static const uint8_t font_P4[4] =
{
    0x7F, 0x09, 0x09, 0x06
};


/* =========================================================
 * COMPACT FONT A
 * ========================================================= */

static const uint8_t font_A4[4] =
{
    0x7E, 0x09, 0x09, 0x7E
};


/* =========================================================
 * COMPACT FONT R
 * ========================================================= */

static const uint8_t font_R4[4] =
{
    0x7F, 0x09, 0x19, 0x46
};


/* =========================================================
 * COMPACT FONT U
 * ========================================================= */

static const uint8_t font_U4[4] =
{
    0x3F, 0x40, 0x40, 0x3F
};


/* =========================================================
 * DRAW COMPACT CHARACTER
 * ========================================================= */

static void OLED_DrawCompactChar(
    uint8_t x,
    uint8_t y,
    char c)
{
    const uint8_t *bitmap;

    uint8_t column;
    uint8_t row;
    uint8_t dx;
    uint8_t dy;


    switch (c)
    {
        case 'T':
            bitmap = font_T4;
            break;

        case 'E':
            bitmap = font_E4;
            break;

        case 'M':
            bitmap = font_M4;
            break;

        case 'P':
            bitmap = font_P4;
            break;

        case 'A':
            bitmap = font_A4;
            break;

        case 'R':
            bitmap = font_R4;
            break;

        case 'U':
            bitmap = font_U4;
            break;

        default:
            return;
    }


    for (column = 0;
         column < 4;
         column++)
    {
        for (row = 0;
             row < 7;
             row++)
        {
            if (bitmap[column] &
                (1U << row))
            {
                for (dx = 0;
                     dx < 2;
                     dx++)
                {
                    for (dy = 0;
                         dy < 2;
                         dy++)
                    {
                        OLED_DrawPixel(
                            x +
                            column * 2 +
                            dx,

                            y +
                            row * 2 +
                            dy
                        );
                    }
                }
            }
        }
    }
}


/* =========================================================
 * DRAW TEMPERATURE TITLE
 * ========================================================= */

static void OLED_DrawTemperatureTitle(
    uint8_t x,
    uint8_t y)
{
    const char *text =
        "TEMPERATURE";


    while (*text)
    {
        OLED_DrawCompactChar(
            x,
            y,
            *text
        );

        x += 10;

        text++;
    }
}


/* =========================================================
 * OLED INITIALIZATION
 * ========================================================= */

static bool OLED_Init(void)
{
    Cy_SysLib_Delay(100);

    if (!OLED_Command(0xAE))
        return false;

    if (!OLED_Command(0xD5))
        return false;

    if (!OLED_Command(0x80))
        return false;

    if (!OLED_Command(0xA8))
        return false;

    if (!OLED_Command(0x3F))
        return false;

    if (!OLED_Command(0xD3))
        return false;

    if (!OLED_Command(0x00))
        return false;

    if (!OLED_Command(0x40))
        return false;

    if (!OLED_Command(0x8D))
        return false;

    if (!OLED_Command(0x14))
        return false;

    if (!OLED_Command(0x20))
        return false;

    if (!OLED_Command(0x00))
        return false;

    if (!OLED_Command(0xA1))
        return false;

    if (!OLED_Command(0xC8))
        return false;

    if (!OLED_Command(0xDA))
        return false;

    if (!OLED_Command(0x12))
        return false;

    if (!OLED_Command(0x81))
        return false;

    if (!OLED_Command(0x7F))
        return false;

    if (!OLED_Command(0xD9))
        return false;

    if (!OLED_Command(0xF1))
        return false;

    if (!OLED_Command(0xDB))
        return false;

    if (!OLED_Command(0x40))
        return false;

    if (!OLED_Command(0xA4))
        return false;

    if (!OLED_Command(0xA6))
        return false;

    if (!OLED_Command(0xAF))
        return false;

    return true;
}


/* =========================================================
 * DISPLAY TEMPERATURE
 * ========================================================= */

static bool OLED_DisplayTemperature(
    int16_t temp_raw)
{
    char temp_string[16];

    int16_t temp_integer;

    uint16_t temp_fraction;


    /* Clear framebuffer */

    OLED_Clear();


    /* TEMPERATURE title */

    OLED_DrawTemperatureTitle(
        9,
        4
    );


    /* Integer */

    temp_integer =
        temp_raw / 16;


    /* Fraction */

    if (temp_raw < 0)
    {
        temp_fraction =
            ((uint16_t)(-temp_raw) % 16)
            * 625;
    }
    else
    {
        temp_fraction =
            ((uint16_t)temp_raw % 16)
            * 625;
    }


    /* Create string */

    snprintf(
        temp_string,
        sizeof(temp_string),
        "%d.%02u C",
        temp_integer,
        (temp_fraction + 500) / 1000
    );


    /* Draw temperature */

    OLED_DrawString(
        22,
        27,
        temp_string,
        2
    );


    /* Update OLED */

    return OLED_Update();
}


/* =========================================================
 * TEMPERATURE APP INIT
 * ========================================================= */

void temperature_app_init(void)
{
    int16_t temp_raw;


    temperature_app_active = true;

    temperature_counter = 0;


    /* Initialize OLED */

    if (!OLED_Init())
    {
        return;
    }


    /* First temperature reading */

    temp_raw =
        TMP102_ReadTemperature();


    if (temp_raw <= -9000)
    {
        return;
    }
}


/* =========================================================
 * TEMPERATURE APP RUN
 * ========================================================= */

void temperature_app_run(void)
{
    int16_t temp_raw;


    if (!temperature_app_active)
    {
        return;
    }


    /*
     * app_manager_run() is called
     * continuously.
     *
     * Update approximately every
     * 1000 calls.
     */

    temperature_counter++;


    if (temperature_counter < 1000)
    {
        return;
    }


    temperature_counter = 0;


    /* Read TMP102 */

    temp_raw =
        TMP102_ReadTemperature();


    if (temp_raw <= -9000)
    {
        return;
    }


    /* Update OLED */

    OLED_DisplayTemperature(
        temp_raw
    );
}


/* =========================================================
 * TEMPERATURE APP STOP
 * ========================================================= */

void temperature_app_stop(void)
{
    temperature_app_active = false;

    temperature_counter = 0;


    /*
     * Turn OLED OFF.
     *
     * No Serial Monitor message.
     */

    OLED_Command(0xAE);
}
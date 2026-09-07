#include "proximity_app.h"

#include "cybsp.h"
#include "cycfg_peripherals.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


/* =========================================================
 * SHARED I2C CONTEXT
 * ========================================================= */

extern cy_stc_scb_i2c_context_t i2c_context;


/* =========================================================
 * I2C
 * ========================================================= */

#define I2C_TIMEOUT             100


/* =========================================================
 * VCNL4200
 * ========================================================= */

#define VCNL4200_ADDR           0x51

#define VCNL4200_PS_CONF1_2     0x03
#define VCNL4200_PS_CONF3_MS     0x04
#define VCNL4200_PS_DATA         0x08
#define VCNL4200_ID              0x0E


/* =========================================================
 * OLED SSD1306
 * ========================================================= */

#define OLED_ADDR               0x3C
#define OLED_WIDTH              128
#define OLED_HEIGHT             64

static uint8_t oled_buffer[
    OLED_WIDTH * OLED_HEIGHT / 8
];


/* =========================================================
 * APPLICATION STATE
 * ========================================================= */

static bool proximity_initialized = false;


/* =========================================================
 * I2C WRITE
 * ========================================================= */

static bool I2C_Write(
    uint8_t device,
    uint8_t *data,
    uint8_t length)
{
    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        device,
        CY_SCB_I2C_WRITE_XFER,
        I2C_TIMEOUT,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return false;
    }

    for (uint8_t i = 0; i < length; i++)
    {
        status = Cy_SCB_I2C_MasterWriteByte(
            I2C_PHR_HW,
            data[i],
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
    }

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        I2C_TIMEOUT,
        &i2c_context
    );

    return true;
}


/* =========================================================
 * I2C REGISTER WRITE
 * ========================================================= */

static bool I2C_WriteRegister(
    uint8_t device,
    uint8_t reg,
    uint8_t *data,
    uint8_t length)
{
    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        device,
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

    for (uint8_t i = 0; i < length; i++)
    {
        status = Cy_SCB_I2C_MasterWriteByte(
            I2C_PHR_HW,
            data[i],
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
    }

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        I2C_TIMEOUT,
        &i2c_context
    );

    return true;
}


/* =========================================================
 * I2C REGISTER READ
 * ========================================================= */

static bool I2C_ReadRegister(
    uint8_t device,
    uint8_t reg,
    uint8_t *data,
    uint8_t length)
{
    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        device,
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
        device,
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

    for (uint8_t i = 0; i < length; i++)
    {
        uint32_t ack;

        if (i == length - 1)
        {
            ack = CY_SCB_I2C_NAK;
        }
        else
        {
            ack = CY_SCB_I2C_ACK;
        }

        status = Cy_SCB_I2C_MasterReadByte(
            I2C_PHR_HW,
            ack,
            &data[i],
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
    }

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        I2C_TIMEOUT,
        &i2c_context
    );

    return true;
}


/* =========================================================
 * VCNL4200 READ ID
 * ========================================================= */

static bool VCNL4200_ReadID(uint16_t *id)
{
    uint8_t data[2];

    if (!I2C_ReadRegister(
            VCNL4200_ADDR,
            VCNL4200_ID,
            data,
            2))
    {
        return false;
    }

    *id =
        ((uint16_t)data[1] << 8) |
        data[0];

    return true;
}


/* =========================================================
 * VCNL4200 INITIALIZATION
 * ========================================================= */

static bool VCNL4200_Init(void)
{
    uint8_t data[2];

    /* PS_CONF1 / PS_CONF2 */

    data[0] = 0x00;
    data[1] = 0x00;

    if (!I2C_WriteRegister(
            VCNL4200_ADDR,
            VCNL4200_PS_CONF1_2,
            data,
            2))
    {
        return false;
    }


    /* PS_CONF3 / MS */

    data[0] = 0x00;
    data[1] = 0x00;

    if (!I2C_WriteRegister(
            VCNL4200_ADDR,
            VCNL4200_PS_CONF3_MS,
            data,
            2))
    {
        return false;
    }

    return true;
}


/* =========================================================
 * READ PROXIMITY
 * ========================================================= */

static bool VCNL4200_ReadProximity(
    uint16_t *proximity)
{
    uint8_t data[2];

    if (!I2C_ReadRegister(
            VCNL4200_ADDR,
            VCNL4200_PS_DATA,
            data,
            2))
    {
        return false;
    }

    /* LSB first */

    *proximity =
        ((uint16_t)data[1] << 8) |
        data[0];

    /* 12-bit proximity */

    *proximity &= 0x0FFF;

    return true;
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
        I2C_TIMEOUT,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return false;
    }

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        0x00,
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
        command,
        I2C_TIMEOUT,
        &i2c_context
    );

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        I2C_TIMEOUT,
        &i2c_context
    );

    return status == CY_SCB_I2C_SUCCESS;
}


/* =========================================================
 * OLED INITIALIZATION
 * ========================================================= */

static void OLED_Init(void)
{
    Cy_SysLib_Delay(100);

    OLED_Command(0xAE);

    OLED_Command(0xD5);
    OLED_Command(0x80);

    OLED_Command(0xA8);
    OLED_Command(0x3F);

    OLED_Command(0xD3);
    OLED_Command(0x00);

    OLED_Command(0x40);

    OLED_Command(0x8D);
    OLED_Command(0x14);

    OLED_Command(0x20);
    OLED_Command(0x00);

    OLED_Command(0xA1);
    OLED_Command(0xC8);

    OLED_Command(0xDA);
    OLED_Command(0x12);

    OLED_Command(0x81);
    OLED_Command(0x7F);

    OLED_Command(0xD9);
    OLED_Command(0xF1);

    OLED_Command(0xDB);
    OLED_Command(0x40);

    OLED_Command(0xA4);
    OLED_Command(0xA6);

    OLED_Command(0xAF);
}


/* =========================================================
 * OLED CLEAR
 * ========================================================= */

static void OLED_Clear(void)
{
    for (uint16_t i = 0;
         i < sizeof(oled_buffer);
         i++)
    {
        oled_buffer[i] = 0x00;
    }
}


/* =========================================================
 * OLED PIXEL
 * ========================================================= */

static void OLED_SetPixel(
    uint8_t x,
    uint8_t y)
{
    if (x >= OLED_WIDTH ||
        y >= OLED_HEIGHT)
    {
        return;
    }

    oled_buffer[
        x + (y / 8) * OLED_WIDTH
    ] |=
        (1 << (y % 8));
}


/* =========================================================
 * FONTS
 * ========================================================= */

static const uint8_t font_digits[10][5] =
{
    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E}
};


/* P R O X I M I T Y */

static const uint8_t font_proximity[8][5] =
{
    {0x7F,0x09,0x09,0x09,0x06},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x63,0x14,0x08,0x14,0x63},
    {0x00,0x41,0x7F,0x41,0x00},
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x07,0x08,0x70,0x08,0x07}
};


/* =========================================================
 * GET FONT
 * ========================================================= */

static const uint8_t *OLED_GetFont(char c)
{
    switch (c)
    {
        case 'P':
            return font_proximity[0];

        case 'R':
            return font_proximity[1];

        case 'O':
            return font_proximity[2];

        case 'X':
            return font_proximity[3];

        case 'I':
            return font_proximity[4];

        case 'M':
            return font_proximity[5];

        case 'T':
            return font_proximity[6];

        case 'Y':
            return font_proximity[7];

        default:
            break;
    }

    if (c >= '0' && c <= '9')
    {
        return font_digits[c - '0'];
    }

    return NULL;
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

    bitmap = OLED_GetFont(c);

    if (bitmap == NULL)
    {
        return;
    }

    for (uint8_t col = 0;
         col < 5;
         col++)
    {
        for (uint8_t row = 0;
             row < 7;
             row++)
        {
            if (bitmap[col] &
                (1 << row))
            {
                for (uint8_t dx = 0;
                     dx < scale;
                     dx++)
                {
                    for (uint8_t dy = 0;
                         dy < scale;
                         dy++)
                    {
                        OLED_SetPixel(
                            x + col * scale + dx,
                            y + row * scale + dy
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
    const char *str,
    uint8_t scale)
{
    while (*str)
    {
        OLED_DrawChar(
            x,
            y,
            *str,
            scale
        );

        x += 6 * scale;

        str++;
    }
}


/* =========================================================
 * OLED UPDATE
 * ========================================================= */

static void OLED_SendPage(uint8_t page)
{
    uint16_t i;

    OLED_Command(0xB0 + page);
    OLED_Command(0x00);
    OLED_Command(0x10);

    Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        OLED_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        I2C_TIMEOUT,
        &i2c_context
    );

    Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        0x40,
        I2C_TIMEOUT,
        &i2c_context
    );

    for (i = 0; i < 128; i++)
    {
        Cy_SCB_I2C_MasterWriteByte(
            I2C_PHR_HW,
            oled_buffer[page * 128 + i],
            I2C_TIMEOUT,
            &i2c_context
        );
    }

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        I2C_TIMEOUT,
        &i2c_context
    );
}


static void OLED_Update(void)
{
    uint8_t page;

    for (page = 0; page < 8; page++)
    {
        OLED_SendPage(page);
    }
}


/* =========================================================
 * DISPLAY PROXIMITY
 * ========================================================= */

static void OLED_DisplayProximity(
    uint16_t proximity)
{
    char value[6];

    uint8_t length;
    uint8_t width;
    uint8_t x;


    OLED_Clear();


    /* PROXIMITY */

    OLED_DrawString(
        37,
        5,
        "PROXIMITY",
        1
    );


    /* Convert value */

    snprintf(
        value,
        sizeof(value),
        "%u",
        proximity
    );


    /* Calculate centered position */

    length = 0;

    while (value[length] != '\0')
    {
        length++;
    }

    width = length * 12;

    if (width < 128)
    {
        x = (128 - width) / 2;
    }
    else
    {
        x = 0;
    }


    OLED_DrawString(
        x,
        28,
        value,
        2
    );


    OLED_Update();
}


/* =========================================================
 * PROXIMITY APPLICATION INIT
 * ========================================================= */

void proximity_app_init(void)
{
    uint16_t sensor_id;

    proximity_initialized = false;

    /*
     * Take control of OLED immediately.
     */

    OLED_Init();

    OLED_Clear();

    OLED_DrawString(
        37,
        5,
        "PROXIMITY",
        1
    );

    OLED_DrawString(
        55,
        28,
        "0",
        2
    );

    OLED_Update();


    /*
     * Check VCNL4200.
     */

    if (!VCNL4200_ReadID(&sensor_id))
    {
        return;
    }


    /*
     * Expected VCNL4200 ID.
     */

    if (sensor_id != 0x5810)
    {
        return;
    }


    /*
     * Initialize proximity sensor.
     */

    if (!VCNL4200_Init())
    {
        return;
    }


    proximity_initialized = true;


    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: PROXIMITY\r\n"
    );
}


/* =========================================================
 * PROXIMITY APPLICATION RUN
 * ========================================================= */

void proximity_app_run(void)
{
    uint16_t proximity;

    if (!proximity_initialized)
    {
        return;
    }

    if (VCNL4200_ReadProximity(&proximity))
    {
        OLED_DisplayProximity(proximity);
    }

    Cy_SysLib_Delay(100);
}


/* =========================================================
 * PROXIMITY APPLICATION STOP
 * ========================================================= */

void proximity_app_stop(void)
{
    /*
     * Do not clear/update the OLED here.
     *
     * That would send another complete
     * 1024-byte framebuffer during an
     * application switch.
     */

    OLED_Command(0xAE);

    proximity_initialized = false;
}
#include "oled_app.h"

#include "cy_pdl.h"
#include "cycfg.h"
#include "cycfg_peripherals.h"

#include <stdint.h>

/* =========================================================
 * OLED configuration
 * ========================================================= */

#define OLED_ADDR       0x3C
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

static cy_stc_scb_i2c_context_t i2c_context;

/* =========================================================
 * I2C write one byte
 * ========================================================= */

static cy_en_scb_i2c_status_t I2C_WriteByte(uint8_t data)
{
    return Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        data,
        1000,
        &i2c_context
    );
}

/* =========================================================
 * Send OLED command
 * ========================================================= */

static void OLED_Command(uint8_t cmd)
{
    Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        OLED_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    I2C_WriteByte(0x00);
    I2C_WriteByte(cmd);

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );
}

/* =========================================================
 * Send one OLED page
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
        1000,
        &i2c_context
    );

    I2C_WriteByte(0x40);

    for (i = 0; i < 128; i++)
    {
        I2C_WriteByte(
            oled_buffer[page * 128 + i]
        );
    }

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );
}

/* =========================================================
 * Update complete OLED
 * ========================================================= */

static void OLED_Update(void)
{
    uint8_t page;

    for (page = 0; page < 8; page++)
    {
        OLED_SendPage(page);
    }
}

/* =========================================================
 * Clear framebuffer
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
 * Draw one pixel
 * ========================================================= */

static void OLED_DrawPixel(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    oled_buffer[x + (y / 8) * OLED_WIDTH] |=
        (1U << (y % 8));
}

/* =========================================================
 * Draw filled rectangle
 * ========================================================= */

static void OLED_FillRect(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height)
{
    uint8_t i;
    uint8_t j;

    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            OLED_DrawPixel(
                x + i,
                y + j
            );
        }
    }
}

/* =========================================================
 * 5x7 font
 * ========================================================= */

static const uint8_t font_H[5] =
{
    0x7F, 0x08, 0x08, 0x08, 0x7F
};

static const uint8_t font_i[5] =
{
    0x00, 0x44, 0x7D, 0x40, 0x00
};

static const uint8_t font_I[5] =
{
    0x00, 0x41, 0x7F, 0x41, 0x00
};

static const uint8_t font_n[5] =
{
    0x7C, 0x08, 0x04, 0x04, 0x78
};

static const uint8_t font_f[5] =
{
    0x08, 0x7E, 0x09, 0x01, 0x02
};

static const uint8_t font_e[5] =
{
    0x38, 0x54, 0x54, 0x54, 0x18
};

static const uint8_t font_o[5] =
{
    0x38, 0x44, 0x44, 0x44, 0x38
};

/* =========================================================
 * Get character bitmap
 * ========================================================= */

static const uint8_t *GetFont(char c)
{
    switch (c)
    {
        case 'H':
            return font_H;

        case 'i':
            return font_i;

        case 'I':
            return font_I;

        case 'n':
            return font_n;

        case 'f':
            return font_f;

        case 'e':
            return font_e;

        case 'o':
            return font_o;

        default:
            return 0;
    }
}

/* =========================================================
 * Draw character
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

    bitmap = GetFont(c);

    if (bitmap == 0)
    {
        return;
    }

    for (column = 0; column < 5; column++)
    {
        for (row = 0; row < 7; row++)
        {
            if (bitmap[column] & (1U << row))
            {
                for (dx = 0; dx < scale; dx++)
                {
                    for (dy = 0; dy < scale; dy++)
                    {
                        OLED_DrawPixel(
                            x + column * scale + dx,
                            y + row * scale + dy
                        );
                    }
                }
            }
        }
    }
}

/* =========================================================
 * Draw string
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

        x += (5 + 1) * scale;
        text++;
    }
}

/* =========================================================
 * Draw smiley face
 * ========================================================= */

static void OLED_DrawSmiley(
    uint8_t cx,
    uint8_t cy)
{
    uint8_t x;
    uint8_t y;

    /* Outer circle */

    for (x = 0; x < 17; x++)
    {
        OLED_DrawPixel(
            cx - 8 + x,
            cy - 8
        );

        OLED_DrawPixel(
            cx - 8 + x,
            cy + 8
        );
    }

    for (y = 0; y < 17; y++)
    {
        OLED_DrawPixel(
            cx - 8,
            cy - 8 + y
        );

        OLED_DrawPixel(
            cx + 8,
            cy - 8 + y
        );
    }

    /* Rounded corners */

    OLED_DrawPixel(cx - 6, cy - 7);
    OLED_DrawPixel(cx + 6, cy - 7);

    OLED_DrawPixel(cx - 7, cy - 6);
    OLED_DrawPixel(cx + 7, cy - 6);

    OLED_DrawPixel(cx - 7, cy + 6);
    OLED_DrawPixel(cx + 7, cy + 6);

    OLED_DrawPixel(cx - 6, cy + 7);
    OLED_DrawPixel(cx + 6, cy + 7);

    /* Eyes */

    OLED_FillRect(
        cx - 5,
        cy - 4,
        2,
        3
    );

    OLED_FillRect(
        cx + 4,
        cy - 4,
        2,
        3
    );

    /* Smile */

    OLED_DrawPixel(cx - 5, cy + 2);
    OLED_DrawPixel(cx - 4, cy + 3);
    OLED_DrawPixel(cx - 3, cy + 4);
    OLED_DrawPixel(cx - 2, cy + 5);
    OLED_DrawPixel(cx - 1, cy + 5);
    OLED_DrawPixel(cx,     cy + 5);
    OLED_DrawPixel(cx + 1, cy + 5);
    OLED_DrawPixel(cx + 2, cy + 5);
    OLED_DrawPixel(cx + 3, cy + 4);
    OLED_DrawPixel(cx + 4, cy + 3);
    OLED_DrawPixel(cx + 5, cy + 2);
}

/* =========================================================
 * SSD1306 initialization
 * ========================================================= */

static void OLED_Init(void)
{
    Cy_SysLib_Delay(100);

    OLED_Command(0xAE);       /* Display OFF */

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

    OLED_Command(0xAF);       /* Display ON */
}

/* =========================================================
 * OLED APP INIT
 * ========================================================= */

void oled_app_init(void)
{
    /* Initialize I2C */

    Cy_SCB_I2C_Init(
        I2C_PHR_HW,
        &I2C_PHR_config,
        &i2c_context
    );

    Cy_SCB_I2C_Enable(
        I2C_PHR_HW,
        &i2c_context
    );

    /* Initialize OLED */

    OLED_Init();

    /* Clear framebuffer */

    OLED_Clear();

    /* -----------------------------------------
     * Hii
     * ----------------------------------------- */

    OLED_DrawString(
        46,
        3,
        "Hii",
        2
    );

    /* -----------------------------------------
     * Infineon
     * ----------------------------------------- */

    OLED_DrawString(
        16,
        21,
        "Infineon",
        2
    );

    /* -----------------------------------------
     * Smiley
     * ----------------------------------------- */

    OLED_DrawSmiley(
        64,
        53
    );

    /* Send framebuffer */

    OLED_Update();
}

/* =========================================================
 * OLED APP RUN
 * ========================================================= */

void oled_app_run(void)
{
    /*
     * Static OLED application.
     *
     * The display has already been updated
     * during oled_app_init().
     */
}

/* =========================================================
 * OLED APP STOP
 * ========================================================= */

void oled_app_stop(void)
{
    /*
     * Clear display when application is stopped.
     */

    OLED_Clear();
    OLED_Update();
}
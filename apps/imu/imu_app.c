#include "imu_app.h"

#include "cybsp.h"
#include "cycfg_peripherals.h"

#include <stdio.h>
#include <stdint.h>


/* =========================================================
 * SHARED I2C CONTEXT
 * ========================================================= */

extern cy_stc_scb_i2c_context_t i2c_context;


/* =========================================================
 * MPU-6050
 * ========================================================= */

#define MPU6050_ADDR        0x68

#define MPU6050_WHO_AM_I   0x75
#define MPU6050_PWR_MGMT1  0x6B

#define MPU6050_ACCEL_XOUT 0x3B
#define MPU6050_GYRO_XOUT  0x43


/* =========================================================
 * OLED SSD1306
 * ========================================================= */

#define OLED_ADDR       0x3C
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

static uint8_t oled_buffer[
    OLED_WIDTH * OLED_HEIGHT / 8
];


/* =========================================================
 * APPLICATION STATE
 * ========================================================= */

static uint8_t imu_initialized = 0;
static uint8_t display_mode = 0;


/* =========================================================
 * MPU-6050 WRITE REGISTER
 * ========================================================= */

static uint8_t MPU6050_WriteRegister(
    uint8_t reg,
    uint8_t data)
{
    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        MPU6050_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return 0;
    }

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        reg,
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

        return 0;
    }

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        data,
        1000,
        &i2c_context
    );

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return 0;
    }

    return 1;
}


/* =========================================================
 * MPU-6050 READ MULTIPLE REGISTERS
 * ========================================================= */

static uint8_t MPU6050_ReadRegisters(
    uint8_t reg,
    uint8_t *data,
    uint8_t length)
{
    uint8_t i;

    cy_en_scb_i2c_status_t status;

    status = Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        MPU6050_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    if (status != CY_SCB_I2C_SUCCESS)
    {
        return 0;
    }

    status = Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        reg,
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

        return 0;
    }

    status = Cy_SCB_I2C_MasterSendReStart(
        I2C_PHR_HW,
        MPU6050_ADDR,
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

        return 0;
    }

    for (i = 0; i < length; i++)
    {
        if (i == (length - 1))
        {
            status = Cy_SCB_I2C_MasterReadByte(
                I2C_PHR_HW,
                CY_SCB_I2C_NAK,
                &data[i],
                1000,
                &i2c_context
            );
        }
        else
        {
            status = Cy_SCB_I2C_MasterReadByte(
                I2C_PHR_HW,
                CY_SCB_I2C_ACK,
                &data[i],
                1000,
                &i2c_context
            );
        }

        if (status != CY_SCB_I2C_SUCCESS)
        {
            Cy_SCB_I2C_MasterSendStop(
                I2C_PHR_HW,
                1000,
                &i2c_context
            );

            return 0;
        }
    }

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );

    return 1;
}


/* =========================================================
 * MPU-6050 INITIALIZATION
 * ========================================================= */

static uint8_t MPU6050_Init(void)
{
    uint8_t who_am_i;

    if (!MPU6050_ReadRegisters(
            MPU6050_WHO_AM_I,
            &who_am_i,
            1))
    {
        return 0;
    }

    if (who_am_i != 0x68)
    {
        return 0;
    }

    /* Wake up MPU-6050 */

    if (!MPU6050_WriteRegister(
            MPU6050_PWR_MGMT1,
            0x00))
    {
        return 0;
    }

    /* Accelerometer: ±2g */

    if (!MPU6050_WriteRegister(
            0x1C,
            0x00))
    {
        return 0;
    }

    /* Gyroscope: ±250 °/s */

    if (!MPU6050_WriteRegister(
            0x1B,
            0x00))
    {
        return 0;
    }

    Cy_SysLib_Delay(100);

    return 1;
}


/* =========================================================
 * MPU-6050 READ SENSOR DATA
 * ========================================================= */

static uint8_t MPU6050_ReadData(
    int16_t *accel_x,
    int16_t *accel_y,
    int16_t *accel_z,
    int16_t *gyro_x,
    int16_t *gyro_y,
    int16_t *gyro_z)
{
    uint8_t data[14];

    if (!MPU6050_ReadRegisters(
            MPU6050_ACCEL_XOUT,
            data,
            14))
    {
        return 0;
    }

    /* Accelerometer */

    *accel_x =
        ((int16_t)data[0] << 8) |
        data[1];

    *accel_y =
        ((int16_t)data[2] << 8) |
        data[3];

    *accel_z =
        ((int16_t)data[4] << 8) |
        data[5];

    /* Gyroscope */

    *gyro_x =
        ((int16_t)data[8] << 8) |
        data[9];

    *gyro_y =
        ((int16_t)data[10] << 8) |
        data[11];

    *gyro_z =
        ((int16_t)data[12] << 8) |
        data[13];

    return 1;
}


/* =========================================================
 * OLED WRITE BYTE
 * ========================================================= */

static void OLED_WriteByte(uint8_t data)
{
    Cy_SCB_I2C_MasterWriteByte(
        I2C_PHR_HW,
        data,
        1000,
        &i2c_context
    );
}


/* =========================================================
 * OLED COMMAND
 * ========================================================= */

static void OLED_Command(
    uint8_t command)
{
    Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        OLED_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    OLED_WriteByte(0x00);
    OLED_WriteByte(command);

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );
}


/* =========================================================
 * OLED SEND PAGE
 * ========================================================= */

static void OLED_SendPage(
    uint8_t page)
{
    uint16_t i;

    OLED_Command(
        0xB0 + page
    );

    OLED_Command(0x00);
    OLED_Command(0x10);

    Cy_SCB_I2C_MasterSendStart(
        I2C_PHR_HW,
        OLED_ADDR,
        CY_SCB_I2C_WRITE_XFER,
        1000,
        &i2c_context
    );

    OLED_WriteByte(0x40);

    for (i = 0; i < 128; i++)
    {
        OLED_WriteByte(
            oled_buffer[
                page * 128 + i
            ]
        );
    }

    Cy_SCB_I2C_MasterSendStop(
        I2C_PHR_HW,
        1000,
        &i2c_context
    );
}


/* =========================================================
 * OLED UPDATE
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
 * OLED CLEAR
 * ========================================================= */

static void OLED_Clear(void)
{
    uint16_t i;

    for (i = 0;
         i < sizeof(oled_buffer);
         i++)
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
    if (x >= 128 ||
        y >= 64)
    {
        return;
    }

    oled_buffer[
        x + ((y / 8) * 128)
    ] |=
        (1 << (y % 8));
}


/* =========================================================
 * FONTS
 * ========================================================= */

static const uint8_t font_A[5] =
{
    0x7E, 0x11, 0x11, 0x11, 0x7E
};

static const uint8_t font_C[5] =
{
    0x3E, 0x41, 0x41, 0x41, 0x22
};

static const uint8_t font_G[5] =
{
    0x3E, 0x41, 0x49, 0x49, 0x7A
};

static const uint8_t font_E[5] =
{
    0x7F, 0x49, 0x49, 0x49, 0x41
};

static const uint8_t font_L[5] =
{
    0x7F, 0x40, 0x40, 0x40, 0x40
};

static const uint8_t font_R[5] =
{
    0x7F, 0x09, 0x19, 0x29, 0x46
};

static const uint8_t font_O[5] =
{
    0x3E, 0x41, 0x41, 0x41, 0x3E
};

static const uint8_t font_M[5] =
{
    0x7F, 0x06, 0x18, 0x06, 0x7F
};

static const uint8_t font_T[5] =
{
    0x01, 0x01, 0x7F, 0x01, 0x01
};

static const uint8_t font_X[5] =
{
    0x63, 0x36, 0x1C, 0x36, 0x63
};

static const uint8_t font_Y[5] =
{
    0x03, 0x0C, 0x70, 0x0C, 0x03
};

static const uint8_t font_Z[5] =
{
    0x61, 0x51, 0x49, 0x45, 0x43
};

static const uint8_t font_0[5] =
{
    0x3E, 0x51, 0x49, 0x45, 0x3E
};

static const uint8_t font_1[5] =
{
    0x00, 0x42, 0x7F, 0x40, 0x00
};

static const uint8_t font_2[5] =
{
    0x42, 0x61, 0x51, 0x49, 0x46
};

static const uint8_t font_3[5] =
{
    0x21, 0x41, 0x45, 0x4B, 0x31
};

static const uint8_t font_4[5] =
{
    0x18, 0x14, 0x12, 0x7F, 0x10
};

static const uint8_t font_5[5] =
{
    0x27, 0x45, 0x45, 0x45, 0x39
};

static const uint8_t font_6[5] =
{
    0x3C, 0x4A, 0x49, 0x49, 0x30
};

static const uint8_t font_7[5] =
{
    0x01, 0x71, 0x09, 0x05, 0x03
};

static const uint8_t font_8[5] =
{
    0x36, 0x49, 0x49, 0x49, 0x36
};

static const uint8_t font_9[5] =
{
    0x06, 0x49, 0x49, 0x29, 0x1E
};

static const uint8_t font_plus[5] =
{
    0x08, 0x08, 0x3E, 0x08, 0x08
};

static const uint8_t font_minus[5] =
{
    0x08, 0x08, 0x08, 0x08, 0x08
};

static const uint8_t font_dot[5] =
{
    0x00, 0x60, 0x60, 0x00, 0x00
};

static const uint8_t font_g[5] =
{
    0x18, 0xA4, 0xA4, 0xA4, 0x7C
};


/* =========================================================
 * GET FONT
 * ========================================================= */

static const uint8_t *OLED_GetFont(
    char c)
{
    switch (c)
    {
        case 'A': return font_A;
        case 'C': return font_C;
        case 'G': return font_G;
        case 'E': return font_E;
        case 'L': return font_L;
        case 'R': return font_R;
        case 'O': return font_O;
        case 'M': return font_M;
        case 'T': return font_T;
        case 'X': return font_X;
        case 'Y': return font_Y;
        case 'Z': return font_Z;

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

        case '+': return font_plus;
        case '-': return font_minus;
        case '.': return font_dot;
        case 'g': return font_g;

        default:
            return 0;
    }
}


/* =========================================================
 * OLED DRAW CHARACTER
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

    if (bitmap == 0)
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
                (1 << row))
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
 * OLED DRAW STRING
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
 * DISPLAY ACCELEROMETER
 * ========================================================= */

static void OLED_DisplayAccelerometer(
    int16_t ax,
    int16_t ay,
    int16_t az)
{
    char line[16];

    int32_t value;

    OLED_Clear();

    OLED_DrawString(
        10,
        2,
        "ACCELEROMETER",
        1
    );

    /* X */

    value = ((int32_t)ax * 100) / 16384;

    snprintf(
        line,
        sizeof(line),
        "X:%ld.%02ldg",
        value / 100,
        (value < 0 ?
         -value : value) % 100
    );

    OLED_DrawString(
        18,
        18,
        line,
        2
    );

    /* Y */

    value = ((int32_t)ay * 100) / 16384;

    snprintf(
        line,
        sizeof(line),
        "Y:%ld.%02ldg",
        value / 100,
        (value < 0 ?
         -value : value) % 100
    );

    OLED_DrawString(
        18,
        33,
        line,
        2
    );

    /* Z */

    value = ((int32_t)az * 100) / 16384;

    snprintf(
        line,
        sizeof(line),
        "Z:%ld.%02ldg",
        value / 100,
        (value < 0 ?
         -value : value) % 100
    );

    OLED_DrawString(
        18,
        48,
        line,
        2
    );

    OLED_Update();
}


/* =========================================================
 * DISPLAY GYROSCOPE
 * ========================================================= */

static void OLED_DisplayGyroscope(
    int16_t gx,
    int16_t gy,
    int16_t gz)
{
    char line[16];

    int32_t value;

    OLED_Clear();

    OLED_DrawString(
        25,
        2,
        "GYROSCOPE",
        1
    );

    /* X */

    value = ((int32_t)gx * 10) / 131;

    snprintf(
        line,
        sizeof(line),
        "X:%ld.%01ld",
        value / 10,
        (value < 0 ?
         -value : value) % 10
    );

    OLED_DrawString(
        18,
        18,
        line,
        2
    );

    /* Y */

    value = ((int32_t)gy * 10) / 131;

    snprintf(
        line,
        sizeof(line),
        "Y:%ld.%01ld",
        value / 10,
        (value < 0 ?
         -value : value) % 10
    );

    OLED_DrawString(
        18,
        33,
        line,
        2
    );

    /* Z */

    value = ((int32_t)gz * 10) / 131;

    snprintf(
        line,
        sizeof(line),
        "Z:%ld.%01ld",
        value / 10,
        (value < 0 ?
         -value : value) % 10
    );

    OLED_DrawString(
        18,
        48,
        line,
        2
    );

    OLED_Update();
}


/* =========================================================
 * IMU APPLICATION INIT
 * ========================================================= */

void imu_app_init(void)
{
    imu_initialized = 0;
    display_mode = 0;

    OLED_Init();

    if (!MPU6050_Init())
    {
        return;
    }

    imu_initialized = 1;

/*    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: IMU\r\n"
    );*/
}


/* =========================================================
 * IMU APPLICATION RUN
 * ========================================================= */

void imu_app_run(void)
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;


    if (!imu_initialized)
    {
        return;
    }


    if (MPU6050_ReadData(
            &accel_x,
            &accel_y,
            &accel_z,
            &gyro_x,
            &gyro_y,
            &gyro_z))
    {
        if (display_mode == 0)
        {
            OLED_DisplayAccelerometer(
                accel_x,
                accel_y,
                accel_z
            );
        }
        else
        {
            OLED_DisplayGyroscope(
                gyro_x,
                gyro_y,
                gyro_z
            );
        }

        display_mode ^= 1;
    }


    /* Same 1 second update rate
     * as the working standalone program.
     */

    Cy_SysLib_Delay(1000);
}


/* =========================================================
 * IMU APPLICATION STOP
 * ========================================================= */

void imu_app_stop(void)
{
    OLED_Command(0xAE);

    imu_initialized = 0;
}
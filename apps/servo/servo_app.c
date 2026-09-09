#include <stdint.h>

#include "cybsp.h"
#include "cy_sysclk.h"
#include "cycfg_peripherals.h"
#include "servo_app.h"

/* ============================================================
 * P6 GPIO REGISTERS
 * ============================================================ */

#define P6_DR           (*(volatile uint32_t *)0x40040600UL)
#define P6_PC           (*(volatile uint32_t *)0x40040608UL)
#define P6_PC2          (*(volatile uint32_t *)0x40040618UL)

/* ============================================================
 * HSIOM
 *
 * P6[4] HSIOM field = bits 19:16
 *
 * TCPWM output = ACTIVE_0 = 8
 * ============================================================ */

#define HSIOM_P6_SEL    (*(volatile uint32_t *)0x40020600UL)

/* ============================================================
 * TCPWM
 * ============================================================ */

#define TCPWM_GLOBAL_CTRL   (*(volatile uint32_t *)0x40200000UL)
#define TCPWM_CMD            (*(volatile uint32_t *)0x40200008UL)

/* ============================================================
 * TCPWM COUNTER 6
 *
 * Counter 6 base = 0x40200280
 * ============================================================ */

#define TCPWM6_CTRL         (*(volatile uint32_t *)0x40200280UL)
#define TCPWM6_STATUS       (*(volatile uint32_t *)0x40200284UL)
#define TCPWM6_COUNTER      (*(volatile uint32_t *)0x40200288UL)
#define TCPWM6_CC           (*(volatile uint32_t *)0x4020028CUL)
#define TCPWM6_PERIOD       (*(volatile uint32_t *)0x40200294UL)
#define TCPWM6_TR_CTRL2     (*(volatile uint32_t *)0x402002A8UL)

/* ============================================================
 * SERVO STATE
 * ============================================================ */

static bool servo_initialized = false;

/* ============================================================
 * SIMPLE DELAY
 * ============================================================ */

static void servo_delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    volatile uint32_t j;

    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 4000UL; j++)
        {
            __asm volatile ("nop");
        }
    }
}

/* ============================================================
 * SERVO SET ANGLE
 *
 * 500 us  = 0°
 * 2500 us = 180°
 *
 * Counter clock = 1 MHz
 * Therefore 1 count = 1 us
 * ============================================================ */

static void servo_set_angle(uint32_t angle)
{
    uint32_t pulse;

    if (angle > 180UL)
    {
        angle = 180UL;
    }

    pulse = 500UL + ((angle * 2000UL) / 180UL);

    TCPWM6_CC = pulse;
}

/* ============================================================
 * SERVO INITIALIZATION
 * ============================================================ */

void servo_app_init(void)
{
    servo_initialized = false;

    /* ========================================================
     * 1. CONFIGURE P6[4]
     *
     * Strong Drive
     * ======================================================== */

    P6_PC &= ~(0x7UL << 12);
    P6_PC |= (0x6UL << 12);

    /* Disable input buffer */

    P6_PC2 &= ~(1UL << 4);

    /* ========================================================
     * 2. ROUTE TCPWM6 OUTPUT TO P6[4]
     *
     * HSIOM ACTIVE_0 = 8
     * ======================================================== */

    HSIOM_P6_SEL &= ~(0xFUL << 16);
    HSIOM_P6_SEL |= (8UL << 16);

    /* ========================================================
     * 3. CONFIGURE TCPWM PERIPHERAL CLOCK DIVIDER 6
     *
     * 48 MHz / 3 = 16 MHz
     * ======================================================== */

    Cy_SysClk_PeriphDisableDivider(
        CY_SYSCLK_DIV_16_BIT,
        6UL
    );

    Cy_SysClk_PeriphAssignDivider(
        PCLK_TCPWM_CLOCKS6,
        CY_SYSCLK_DIV_16_BIT,
        6UL
    );

    Cy_SysClk_PeriphSetDivider(
        CY_SYSCLK_DIV_16_BIT,
        6UL,
        2UL
    );

    Cy_SysClk_PeriphEnableDivider(
        CY_SYSCLK_DIV_16_BIT,
        6UL
    );

    /* ========================================================
     * 4. DISABLE TCPWM COUNTER 6
     * ======================================================== */

    TCPWM_GLOBAL_CTRL &= ~(1UL << 6);

    /* ========================================================
     * 5. CONFIGURE TCPWM COUNTER 6
     *
     * PWM mode
     * Up counting
     * Prescaler /16
     * Continuous mode
     * ======================================================== */

    TCPWM6_CTRL =
          (4UL << 24)
        | (0UL << 16)
        | (4UL << 8)
        | (1UL << 1)
        | (1UL << 0);

    /* ========================================================
     * 6. PWM PERIOD
     *
     * 1 MHz counter clock
     * 20 ms = 20,000 us
     *
     * Counter = 0 -> 19999
     * ======================================================== */

    TCPWM6_PERIOD = 19999UL;

    /* ========================================================
     * 7. PWM OUTPUT
     *
     * Overflow -> SET
     * CC match -> CLEAR
     * Underflow -> NO CHANGE
     * ======================================================== */

    TCPWM6_TR_CTRL2 =
          (0UL << 4)
        | (0UL << 2)
        | (1UL << 0);

    /* ========================================================
     * 8. INITIAL POSITION
     *
     * 0° = 500 us
     * ======================================================== */

    servo_set_angle(0UL);

    /* ========================================================
     * 9. ENABLE TCPWM COUNTER 6
     * ======================================================== */

    TCPWM_GLOBAL_CTRL |= (1UL << 6);

    /* ========================================================
     * 10. START TCPWM COUNTER 6
     * ======================================================== */

    TCPWM_CMD = (1UL << 30);

    servo_initialized = true;

    /* ========================================================
     * SUCCESS MESSAGE
     * ======================================================== */

    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: SERVO\r\n"
    );
}

/* ============================================================
 * SERVO APPLICATION RUN
 * ============================================================ */

void servo_app_run(void)
{
    if (!servo_initialized)
    {
        return;
    }

    /* 0° */
    servo_set_angle(0UL);
    servo_delay_ms(1000UL);

    /* 75° */
    servo_set_angle(75UL);
    servo_delay_ms(1000UL);

    /* 110° */
    servo_set_angle(110UL);
    servo_delay_ms(1000UL);

    /* 135° */
    servo_set_angle(135UL);
    servo_delay_ms(1000UL);

    /* 180° */
    servo_set_angle(180UL);
    servo_delay_ms(1000UL);
}

/* ============================================================
 * SERVO APPLICATION STOP
 * ============================================================ */

void servo_app_stop(void)
{
    /* Disable TCPWM counter 6 */

    TCPWM_GLOBAL_CTRL &= ~(1UL << 6);

    servo_initialized = false;
}
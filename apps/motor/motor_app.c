#include <stdint.h>
#include <stdbool.h>

#include "cy_pdl.h"
#include "cybsp.h"

#include "motor_app.h"


/*
 * ============================================================
 * PSoC 4100S Plus
 * CY8C4147AZI-S475
 *
 * TB6612FNG MOTOR DRIVER
 *
 * PWMA  -> P6[0]
 * AIN1  -> P6[1]
 * AIN2  -> P6[2]
 *
 * Motor -> AO1 / AO2
 *
 *
 * PWM:
 *
 * System clock       = 48 MHz
 * Peripheral divider = /1
 * TCPWM prescaler    = /1
 *
 * TCPWM clock        = 48 MHz
 *
 * PWM frequency      = 20 kHz
 *
 * Period:
 *
 * 48 MHz / 20 kHz = 2400 counts
 *
 * PERIOD = 2399
 *
 * ============================================================
 */


/* ============================================================
 * P6 GPIO REGISTERS
 * ============================================================ */

#define P6_DR           (*(volatile uint32_t *)0x40040600UL)
#define P6_PC           (*(volatile uint32_t *)0x40040608UL)
#define P6_PC2          (*(volatile uint32_t *)0x40040618UL)


/* ============================================================
 * HSIOM
 *
 * P6[0] field = bits 3:0
 *
 * TCPWM output = ACTIVE_0 = 8
 * ============================================================ */

#define HSIOM_P6_SEL    (*(volatile uint32_t *)0x40020600UL)


/* ============================================================
 * PERIPHERAL CLOCK
 *
 * PCLK_TCPWM_CLOCKS4 = PCLK10
 *
 * Divider 4 is used for TCPWM Counter 4
 * ============================================================ */

#define PERI_DIV_CMD        (*(volatile uint32_t *)0x40010000UL)

#define PERI_PCLK_CTL10     (*(volatile uint32_t *)0x40010128UL)

#define PERI_DIV_16_4       (*(volatile uint32_t *)0x40010310UL)


/* ============================================================
 * TCPWM GLOBAL REGISTERS
 * ============================================================ */

#define TCPWM_GLOBAL_CTRL   (*(volatile uint32_t *)0x40200000UL)

#define TCPWM_CMD           (*(volatile uint32_t *)0x40200008UL)


/* ============================================================
 * TCPWM COUNTER 4
 *
 * Counter 4 base:
 *
 * 0x40200200
 * ============================================================ */

#define TCPWM4_CTRL         (*(volatile uint32_t *)0x40200200UL)

#define TCPWM4_STATUS       (*(volatile uint32_t *)0x40200204UL)

#define TCPWM4_COUNTER      (*(volatile uint32_t *)0x40200208UL)

#define TCPWM4_CC           (*(volatile uint32_t *)0x4020020CUL)

#define TCPWM4_PERIOD       (*(volatile uint32_t *)0x40200214UL)

#define TCPWM4_TR_CTRL2     (*(volatile uint32_t *)0x40200228UL)


/* ============================================================
 * APPLICATION STATE
 * ============================================================ */

static bool motor_initialized = false;


/* ============================================================
 * DELAY
 * ============================================================ */

static void delay_ms(uint32_t ms)
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
 * MOTOR DIRECTION
 *
 * AIN1 = P6[1]
 * AIN2 = P6[2]
 * ============================================================ */

static void motor_forward(void)
{
    /* AIN1 = 1 */
    P6_DR |= (1UL << 1);

    /* AIN2 = 0 */
    P6_DR &= ~(1UL << 2);
}


static void motor_reverse(void)
{
    /* AIN1 = 0 */
    P6_DR &= ~(1UL << 1);

    /* AIN2 = 1 */
    P6_DR |= (1UL << 2);
}


static void motor_stop(void)
{
    /* AIN1 = 0 */
    P6_DR &= ~(1UL << 1);

    /* AIN2 = 0 */
    P6_DR &= ~(1UL << 2);
}


static void motor_brake(void)
{
    /* AIN1 = 1 */
    P6_DR |= (1UL << 1);

    /* AIN2 = 1 */
    P6_DR |= (1UL << 2);
}


/* ============================================================
 * SET MOTOR PWM DUTY
 *
 * 0   = 0%
 * 25  = 25%
 * 50  = 50%
 * 75  = 75%
 * 100 = 100%
 *
 * PWM period = 2400 counts
 * ============================================================ */

static void motor_set_pwm(uint32_t duty)
{
    uint32_t compare;

    if (duty > 100UL)
    {
        duty = 100UL;
    }

    compare = (2400UL * duty) / 100UL;

    TCPWM4_CC = compare;
}


/* ============================================================
 * MOTOR PWM INITIALIZATION
 * ============================================================ */

static void motor_pwm_init(void)
{
    /* ========================================================
     * 1. P6[0] -> TCPWM4 PWM OUTPUT
     *
     * ACTIVE_0 = 8
     * ======================================================== */

    HSIOM_P6_SEL &= ~(0xFUL << 0);

    HSIOM_P6_SEL |= (8UL << 0);


    /* ========================================================
     * 2. P6[0] STRONG DRIVE
     * ======================================================== */

    P6_PC &= ~(7UL << 0);

    P6_PC |= (6UL << 0);


    /* Disable input buffer */

    P6_PC2 |= (1UL << 0);


    /* ========================================================
     * 3. P6[1] = AIN1
     * P6[2] = AIN2
     *
     * Configure both as strong GPIO outputs
     * ======================================================== */

    /* P6[1] */

    P6_PC &= ~(7UL << 3);

    P6_PC |= (6UL << 3);


    /* P6[2] */

    P6_PC &= ~(7UL << 6);

    P6_PC |= (6UL << 6);


    /* Disable input buffers */

    P6_PC2 |= (1UL << 1);
    P6_PC2 |= (1UL << 2);


    /* ========================================================
     * 4. INITIAL MOTOR STATE
     *
     * Stop motor before enabling PWM
     * ======================================================== */

    motor_stop();


    /* ========================================================
     * 5. CONFIGURE PERIPHERAL CLOCK
     *
     * PCLK10 -> Divider 4
     *
     * Divider 4 = /1
     *
     * 48 MHz / 1 = 48 MHz
     * ======================================================== */

    PERI_DIV_16_4 = 1UL;


    /* Assign Divider 4 to PCLK10 */

    PERI_PCLK_CTL10 =
          (1UL << 6)
        | 4UL;


    /* Enable Divider 4 */

    PERI_DIV_CMD =
          (1UL << 31)
        | (3UL << 14)
        | (63UL << 8)
        | (1UL << 6)
        | 4UL;


    /* ========================================================
     * 6. DISABLE TCPWM COUNTER 4
     * ======================================================== */

    TCPWM_GLOBAL_CTRL &= ~(1UL << 4);


    /* ========================================================
     * 7. CONFIGURE TCPWM COUNTER 4
     *
     * PWM mode
     * Up counting
     * Prescaler /1
     * Auto reload period
     * Auto reload compare
     * ======================================================== */

    TCPWM4_CTRL =
          (4UL << 24)       /* PWM mode */
        | (0UL << 16)       /* Up counting */
        | (0UL << 8)        /* Prescaler /1 */
        | (1UL << 1)        /* Reload period */
        | (1UL << 0);       /* Reload compare */


    /* ========================================================
     * 8. PWM PERIOD
     *
     * 48 MHz / 20 kHz = 2400
     *
     * Counter:
     *
     * 0 -> 2399
     * ======================================================== */

    TCPWM4_PERIOD = 2399UL;


    /* ========================================================
     * 9. PWM OUTPUT ACTION
     *
     * Overflow -> SET
     * CC match -> CLEAR
     * Underflow -> NO CHANGE
     * ======================================================== */

    TCPWM4_TR_CTRL2 =
          (3UL << 4)        /* Underflow: NO CHANGE */
        | (0UL << 2)        /* Overflow: SET */
        | (1UL << 0);       /* CC match: CLEAR */


    /* ========================================================
     * 10. INITIAL DUTY
     *
     * 0%
     * ======================================================== */

    motor_set_pwm(0UL);


    /* ========================================================
     * 11. ENABLE TCPWM COUNTER 4
     * ======================================================== */

    TCPWM_GLOBAL_CTRL |= (1UL << 4);


    /* ========================================================
     * 12. START TCPWM COUNTER 4
     *
     * Counter 4 -> bit 28
     * ======================================================== */

    TCPWM_CMD = (1UL << 28);
}


/* ============================================================
 * TRAINER KIT APPLICATION INIT
 * ============================================================ */

void motor_app_init(void)
{
    motor_initialized = false;

    motor_pwm_init();

    motor_initialized = true;

    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: MOTOR\r\n"
    );
}


/* ============================================================
 * TRAINER KIT APPLICATION RUN
 * ============================================================ */

void motor_app_run(void)
{
    if (!motor_initialized)
    {
        return;
    }


    /* ========================================================
     * FORWARD
     *
     * 0% -> 25% -> 50% -> 75% -> 100%
     * ======================================================== */

    motor_forward();


    /* 0% */

    motor_set_pwm(0UL);
    delay_ms(1000UL);


    /* 25% */

    motor_set_pwm(25UL);
    delay_ms(2000UL);


    /* 50% */

    motor_set_pwm(50UL);
    delay_ms(2000UL);


    /* 75% */

    motor_set_pwm(75UL);
    delay_ms(2000UL);


    /* 100% */

    motor_set_pwm(100UL);
    delay_ms(2000UL);


    /* Stop */

    motor_set_pwm(0UL);
    motor_stop();
    delay_ms(1000UL);


    /* ========================================================
     * REVERSE
     *
     * 25% -> 50% -> 75% -> 100%
     * ======================================================== */

    motor_reverse();


    /* 25% */

    motor_set_pwm(25UL);
    delay_ms(2000UL);


    /* 50% */

    motor_set_pwm(50UL);
    delay_ms(2000UL);


    /* 75% */

    motor_set_pwm(75UL);
    delay_ms(2000UL);


    /* 100% */

    motor_set_pwm(100UL);
    delay_ms(2000UL);


    /* Stop */

    motor_set_pwm(0UL);
    motor_stop();
    delay_ms(1000UL);
}


/* ============================================================
 * TRAINER KIT APPLICATION STOP
 * ============================================================ */

void motor_app_stop(void)
{
    /*
     * Immediately stop PWM and motor.
     */

    motor_set_pwm(0UL);

    motor_stop();

    motor_initialized = false;
}
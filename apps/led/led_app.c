#include "led_app.h"

#include <stdint.h>

/* ============================================================
 * P2 GPIO REGISTERS
 * ============================================================ */

#define GPIO_PRT2_DR          (*(volatile uint32_t *)0x40040200UL)
#define GPIO_PRT2_PC          (*(volatile uint32_t *)0x40040208UL)
#define GPIO_PRT2_PC2         (*(volatile uint32_t *)0x40040218UL)

#define GPIO_PRT2_DR_SET      (*(volatile uint32_t *)0x40040240UL)
#define GPIO_PRT2_DR_CLR      (*(volatile uint32_t *)0x40040244UL)


/* ============================================================
 * P3 GPIO REGISTERS
 * ============================================================ */

#define GPIO_PRT3_DR          (*(volatile uint32_t *)0x40040300UL)
#define GPIO_PRT3_PC          (*(volatile uint32_t *)0x40040308UL)
#define GPIO_PRT3_PC2         (*(volatile uint32_t *)0x40040318UL)

#define GPIO_PRT3_DR_SET      (*(volatile uint32_t *)0x40040340UL)
#define GPIO_PRT3_DR_CLR      (*(volatile uint32_t *)0x40040344UL)


/* ============================================================
 * HSIOM REGISTERS
 * ============================================================ */

#define HSIOM_PORT_SEL2       (*(volatile uint32_t *)0x40020200UL)
#define HSIOM_PORT_SEL3       (*(volatile uint32_t *)0x40020300UL)


/* ============================================================
 * PIN DEFINITIONS
 * ============================================================ */

#define P2_7_PIN              7U
#define P3_3_PIN              3U
#define P3_5_PIN              5U

#define P2_7_MASK             (1UL << P2_7_PIN)
#define P3_3_MASK             (1UL << P3_3_PIN)
#define P3_5_MASK             (1UL << P3_5_PIN)


/* ============================================================
 * GPIO DRIVE MODE
 * ============================================================ */

#define GPIO_DM_STRONG        0x06UL


/* P2_7 drive mode */

#define P2_7_DM_SHIFT         21U
#define P2_7_DM_MASK          (0x07UL << P2_7_DM_SHIFT)


/* P3_3 drive mode */

#define P3_3_DM_SHIFT         9U
#define P3_3_DM_MASK          (0x07UL << P3_3_DM_SHIFT)


/* P3_5 drive mode */

#define P3_5_DM_SHIFT         15U
#define P3_5_DM_MASK          (0x07UL << P3_5_DM_SHIFT)


/* ============================================================
 * HSIOM GPIO FUNCTION
 * ============================================================ */

#define HSIOM_GPIO            0x00UL

#define P2_7_HSIOM_SHIFT      28U
#define P2_7_HSIOM_MASK       (0x0FUL << P2_7_HSIOM_SHIFT)

#define P3_3_HSIOM_SHIFT      12U
#define P3_3_HSIOM_MASK       (0x0FUL << P3_3_HSIOM_SHIFT)

#define P3_5_HSIOM_SHIFT      20U
#define P3_5_HSIOM_MASK       (0x0FUL << P3_5_HSIOM_SHIFT)


/* ============================================================
 * DELAY
 * ============================================================ */

static void led_delay(uint32_t delay)
{
    volatile uint32_t i,j;

    for(j = 0; j < delay; j++)
    {
        for (i = 0; i < 300000UL; i++)
        {
            __asm volatile ("nop");
        }
    }
}


/* ============================================================
 * LED INITIALIZATION
 * ============================================================ */

void led_app_init(void)
{
    /* Configure P2_7 as Strong Drive GPIO */

    GPIO_PRT2_PC &= ~P2_7_DM_MASK;

    GPIO_PRT2_PC |=
        (GPIO_DM_STRONG << P2_7_DM_SHIFT);


    /* Configure P3_3 as Strong Drive GPIO */

    GPIO_PRT3_PC &= ~P3_3_DM_MASK;

    GPIO_PRT3_PC |=
        (GPIO_DM_STRONG << P3_3_DM_SHIFT);


    /* Configure P3_5 as Strong Drive GPIO */

    GPIO_PRT3_PC &= ~P3_5_DM_MASK;

    GPIO_PRT3_PC |=
        (GPIO_DM_STRONG << P3_5_DM_SHIFT);


    /* Enable input buffer */

    GPIO_PRT2_PC2 &= ~P2_7_MASK;

    GPIO_PRT3_PC2 &= ~P3_3_MASK;

    GPIO_PRT3_PC2 &= ~P3_5_MASK;


    /* Connect pins to GPIO through HSIOM */

    HSIOM_PORT_SEL2 &= ~P2_7_HSIOM_MASK;

    HSIOM_PORT_SEL2 |=
        (HSIOM_GPIO << P2_7_HSIOM_SHIFT);


    HSIOM_PORT_SEL3 &= ~P3_3_HSIOM_MASK;

    HSIOM_PORT_SEL3 |=
        (HSIOM_GPIO << P3_3_HSIOM_SHIFT);


    HSIOM_PORT_SEL3 &= ~P3_5_HSIOM_MASK;

    HSIOM_PORT_SEL3 |=
        (HSIOM_GPIO << P3_5_HSIOM_SHIFT);


    /* Initially turn LEDs OFF */

    GPIO_PRT2_DR_CLR = P2_7_MASK;

    GPIO_PRT3_DR_CLR = P3_3_MASK;

    GPIO_PRT3_DR_CLR = P3_5_MASK;
}


/* ============================================================
 * LED RUN
 * ============================================================ */

void led_app_run(void)
{
    GPIO_PRT3_DR_SET = P3_5_MASK;
    led_delay(5);
    GPIO_PRT3_DR_CLR = P3_5_MASK;
    
    GPIO_PRT3_DR_SET = P3_3_MASK;
    led_delay(5);
    GPIO_PRT3_DR_CLR = P3_3_MASK;
    
    GPIO_PRT2_DR_SET = P2_7_MASK;
    led_delay(5);
    GPIO_PRT2_DR_CLR = P2_7_MASK;

    GPIO_PRT3_DR_SET = P3_3_MASK;
    led_delay(5);
    GPIO_PRT3_DR_CLR = P3_3_MASK;

    // GPIO_PRT3_DR_SET = P3_5_MASK;
    // led_delay(1);
    // GPIO_PRT3_DR_CLR = P3_5_MASK;
}


/* ============================================================
 * LED STOP
 * ============================================================ */

void led_app_stop(void)
{
    GPIO_PRT2_DR_CLR = P2_7_MASK;

    GPIO_PRT3_DR_CLR = P3_3_MASK;

    GPIO_PRT3_DR_CLR = P3_5_MASK;
}
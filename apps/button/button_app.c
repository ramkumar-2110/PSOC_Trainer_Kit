#include "button_app.h"

#include <stdint.h>

/* ============================================================
 * P1 REGISTERS
 * ============================================================ */

#define GPIO_PRT1_PS          (*(volatile uint32_t *)0x40040104UL)
#define GPIO_PRT1_PC          (*(volatile uint32_t *)0x40040108UL)
#define GPIO_PRT1_PC2         (*(volatile uint32_t *)0x40040118UL)


/* ============================================================
 * P2 REGISTERS
 * ============================================================ */

#define GPIO_PRT2_DR_SET      (*(volatile uint32_t *)0x40040240UL)
#define GPIO_PRT2_DR_CLR      (*(volatile uint32_t *)0x40040244UL)
#define GPIO_PRT2_PC          (*(volatile uint32_t *)0x40040208UL)
#define GPIO_PRT2_PC2         (*(volatile uint32_t *)0x40040218UL)


/* ============================================================
 * P3 REGISTERS
 * ============================================================ */

#define GPIO_PRT3_DR_SET      (*(volatile uint32_t *)0x40040340UL)
#define GPIO_PRT3_DR_CLR      (*(volatile uint32_t *)0x40040344UL)
#define GPIO_PRT3_PC          (*(volatile uint32_t *)0x40040308UL)
#define GPIO_PRT3_PC2         (*(volatile uint32_t *)0x40040318UL)


/* ============================================================
 * HSIOM REGISTERS
 * ============================================================ */

#define HSIOM_PORT_SEL1       (*(volatile uint32_t *)0x40020100UL)
#define HSIOM_PORT_SEL2       (*(volatile uint32_t *)0x40020200UL)
#define HSIOM_PORT_SEL3       (*(volatile uint32_t *)0x40020300UL)


/* ============================================================
 * PIN MASKS
 * ============================================================ */

#define BUTTON_MASK           (1UL << 6)

#define LED1_MASK             (1UL << 5)
#define LED2_MASK             (1UL << 3)
#define LED3_MASK             (1UL << 7)


/* ============================================================
 * DRIVE MODE
 * ============================================================ */

#define GPIO_DM_STRONG        0x06UL
#define GPIO_DM_HIGHZ         0x01UL


/* ============================================================
 * DRIVE MODE SHIFTS
 * ============================================================ */

#define BUTTON_DM_SHIFT       18U
#define BUTTON_DM_MASK        (0x07UL << BUTTON_DM_SHIFT)

#define LED1_DM_SHIFT         15U
#define LED1_DM_MASK          (0x07UL << LED1_DM_SHIFT)

#define LED2_DM_SHIFT         9U
#define LED2_DM_MASK          (0x07UL << LED2_DM_SHIFT)

#define LED3_DM_SHIFT         21U
#define LED3_DM_MASK          (0x07UL << LED3_DM_SHIFT)


/* ============================================================
 * HSIOM MASKS
 * ============================================================ */

#define BUTTON_HSIOM_MASK     (0x0FUL << 24U)

#define LED1_HSIOM_MASK       (0x0FUL << 20U)

#define LED2_HSIOM_MASK       (0x0FUL << 12U)

#define LED3_HSIOM_MASK       (0x0FUL << 28U)


/* ============================================================
 * BUTTON STATE
 * ============================================================ */

static uint8_t led_state = 0U;

static uint8_t button_was_pressed = 0U;


/* ============================================================
 * TURN ALL LEDs OFF
 * ============================================================ */

static void LEDs_Off(void)
{
    GPIO_PRT3_DR_CLR = LED1_MASK;
    GPIO_PRT3_DR_CLR = LED2_MASK;
    GPIO_PRT2_DR_CLR = LED3_MASK;
}


/* ============================================================
 * BUTTON APPLICATION INITIALIZATION
 * ============================================================ */

void button_app_init(void)
{
    /*
     * Configure P1_6 as High-Z digital input.
     */

    GPIO_PRT1_PC &= ~BUTTON_DM_MASK;

    GPIO_PRT1_PC |=
        (GPIO_DM_HIGHZ << BUTTON_DM_SHIFT);

    GPIO_PRT1_PC2 &= ~BUTTON_MASK;

    HSIOM_PORT_SEL1 &= ~BUTTON_HSIOM_MASK;


    /*
     * Configure LED1 P3_5.
     */

    GPIO_PRT3_PC &= ~LED1_DM_MASK;

    GPIO_PRT3_PC |=
        (GPIO_DM_STRONG << LED1_DM_SHIFT);

    GPIO_PRT3_PC2 &= ~LED1_MASK;

    HSIOM_PORT_SEL3 &= ~LED1_HSIOM_MASK;


    /*
     * Configure LED2 P3_3.
     */

    GPIO_PRT3_PC &= ~LED2_DM_MASK;

    GPIO_PRT3_PC |=
        (GPIO_DM_STRONG << LED2_DM_SHIFT);

    GPIO_PRT3_PC2 &= ~LED2_MASK;

    HSIOM_PORT_SEL3 &= ~LED2_HSIOM_MASK;


    /*
     * Configure LED3 P2_7.
     */

    GPIO_PRT2_PC &= ~LED3_DM_MASK;

    GPIO_PRT2_PC |=
        (GPIO_DM_STRONG << LED3_DM_SHIFT);

    GPIO_PRT2_PC2 &= ~LED3_MASK;

    HSIOM_PORT_SEL2 &= ~LED3_HSIOM_MASK;


    /*
     * Initial state.
     */

    led_state = 0U;

    button_was_pressed = 0U;

    LEDs_Off();
}


/* ============================================================
 * BUTTON APPLICATION RUN
 * ============================================================ */

void button_app_run(void)
{
    uint8_t button_pressed;

    /*
     * Button is active LOW.
     *
     * 0 = pressed
     * 1 = released
     */

    button_pressed =
        ((GPIO_PRT1_PS & BUTTON_MASK) == 0U);


    /*
     * Detect a new button press.
     *
     * We only react when the button changes
     * from released -> pressed.
     */

    if (button_pressed && !button_was_pressed)
    {
        /*
         * Move to next LED.
         */

        led_state++;

        if (led_state > 3U)
        {
            led_state = 1U;
        }


        /*
         * Turn all LEDs OFF.
         */

        LEDs_Off();


        /*
         * Select LED.
         */

        if (led_state == 1U)
        {
            GPIO_PRT3_DR_SET = LED1_MASK;
        }
        else if (led_state == 2U)
        {
            GPIO_PRT3_DR_SET = LED2_MASK;
        }
        else if (led_state == 3U)
        {
            GPIO_PRT2_DR_SET = LED3_MASK;
        }
    }


    /*
     * Remember current button state.
     */

    button_was_pressed = button_pressed;
}


/* ============================================================
 * BUTTON APPLICATION STOP
 * ============================================================ */

void button_app_stop(void)
{
    LEDs_Off();

    led_state = 0U;

    button_was_pressed = 0U;
}
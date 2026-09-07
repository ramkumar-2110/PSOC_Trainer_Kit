/*

 * File Name : main.c

 *

 * Description:

 * Bare-metal button controlled relay operation

 * using direct memory-mapped GPIO register access.

 *

 * Buttons:

 * Button 1 -> P1_6

 * Button 2 -> P3_7

 *

 * Relays:

 * Relay 1 -> P2_5

 * Relay 2 -> P1_7

 *

 * Button 1:

 * External pull-up resistor

 * Released = LOW

 * Pressed  = HIGH

 *

 * Target:

 * CY8C4147AZI-S475

 */

#include <stdint.h>
#include "relay_app.h"

/* ============================================================

 * P1 GPIO REGISTERS

 * ============================================================ */

/* Port Status Register */

#define GPIO_PRT1_PS          (*(volatile uint32_t *)0x40040104UL)

/* Port Configuration Register */

#define GPIO_PRT1_PC          (*(volatile uint32_t *)0x40040108UL)

/* Port Configuration Register 2 */

#define GPIO_PRT1_PC2         (*(volatile uint32_t *)0x40040118UL)

/* Port Data Register SET */

#define GPIO_PRT1_DR_SET      (*(volatile uint32_t *)0x40040140UL)

/* Port Data Register CLEAR */

#define GPIO_PRT1_DR_CLR      (*(volatile uint32_t *)0x40040144UL)


/* ============================================================

 * P2 GPIO REGISTERS

 * ============================================================ */

/* Port Configuration Register */

#define GPIO_PRT2_PC          (*(volatile uint32_t *)0x40040208UL)

/* Port Configuration Register 2 */

#define GPIO_PRT2_PC2         (*(volatile uint32_t *)0x40040218UL)

/* Port Data Register SET */

#define GPIO_PRT2_DR_SET      (*(volatile uint32_t *)0x40040240UL)

/* Port Data Register CLEAR */

#define GPIO_PRT2_DR_CLR      (*(volatile uint32_t *)0x40040244UL)


/* ============================================================

 * HSIOM REGISTERS

 * ============================================================ */

/* HSIOM Port Select 1 */

#define HSIOM_PORT_SEL1       (*(volatile uint32_t *)0x40020100UL)

/* HSIOM Port Select 2 */

#define HSIOM_PORT_SEL2       (*(volatile uint32_t *)0x40020200UL)


/* ============================================================

 * PIN DEFINITIONS

 * ============================================================ */

/* Button 1 -> P1_6 */

#define BUTTON1_MASK          (1UL << 6)

/* Relay 1 -> P2_5 */

#define RELAY1_MASK           (1UL << 5)

/* Relay 2 -> P1_7 */

#define RELAY2_MASK           (1UL << 7)


/* ============================================================

 * GPIO DRIVE MODES

 * ============================================================ */

/*

 * High-Z digital input

 *

 * Used for P1_6 because an external

 * pull-down resistor is present.

 */

#define GPIO_DM_HIGHZ         0x01UL

/*

 * Strong Drive

 *

 * Used for relay outputs.

 */

#define GPIO_DM_STRONG        0x06UL


/* ============================================================

 * BUTTON 1 : P1_6

 * ============================================================ */

/*

 * Each pin has a 3-bit drive-mode field.

 *

 * P1_6:

 *

 * 6 × 3 = 18

 *

 * Bits 20:18

 */

#define BUTTON1_DM_SHIFT      18U

#define BUTTON1_DM_MASK       (0x07UL << BUTTON1_DM_SHIFT)


/*

 * P1_6 HSIOM:

 *

 * 6 × 4 = 24

 *

 * Bits 27:24

 */

#define BUTTON1_HSIOM_SHIFT   24U

#define BUTTON1_HSIOM_MASK    (0x0FUL << BUTTON1_HSIOM_SHIFT)

/* ============================================================

 * RELAY 1 : P2_5

 * ============================================================ */

/*

 * P2_5:

 *

 * 5 × 3 = 15

 *

 * Bits 17:15

 */

#define RELAY1_DM_SHIFT       15U

#define RELAY1_DM_MASK        (0x07UL << RELAY1_DM_SHIFT)


/*

 * P2_5 HSIOM:

 *

 * 5 × 4 = 20

 *

 * Bits 23:20

 */

#define RELAY1_HSIOM_SHIFT    20U

#define RELAY1_HSIOM_MASK     (0x0FUL << RELAY1_HSIOM_SHIFT)


/* ============================================================

 * RELAY 2 : P1_7

 * ============================================================ */

/*

 * P1_7:

 *

 * 7 × 3 = 21

 *

 * Bits 23:21

 */

#define RELAY2_DM_SHIFT       21U

#define RELAY2_DM_MASK        (0x07UL << RELAY2_DM_SHIFT)


/*

 * P1_7 HSIOM:

 *

 * 7 × 4 = 28

 *

 * Bits 31:28

 */

#define RELAY2_HSIOM_SHIFT    28U

#define RELAY2_HSIOM_MASK     (0x0FUL << RELAY2_HSIOM_SHIFT)


/* ============================================================

 * HSIOM GPIO FUNCTION

 * ============================================================ */

#define HSIOM_GPIO            0x00UL


/* ============================================================

 * DELAY FUNCTION

 * ============================================================ */

/*

 * Small software delay for button debounce.

 */

void relay_app_init(void)
{
    /* P1_6 as High-Z digital input */
    GPIO_PRT1_PC &= ~BUTTON1_DM_MASK;
    GPIO_PRT1_PC |=
        (GPIO_DM_HIGHZ << BUTTON1_DM_SHIFT);

    /* Enable input buffer */
    GPIO_PRT1_PC2 &= ~BUTTON1_MASK;

    /* P1_6 -> GPIO */
    HSIOM_PORT_SEL1 &= ~BUTTON1_HSIOM_MASK;
    HSIOM_PORT_SEL1 |=
        (HSIOM_GPIO << BUTTON1_HSIOM_SHIFT);

    /* P2_5 -> Strong Drive */
    GPIO_PRT2_PC &= ~RELAY1_DM_MASK;
    GPIO_PRT2_PC |=
        (GPIO_DM_STRONG << RELAY1_DM_SHIFT);

    HSIOM_PORT_SEL2 &= ~RELAY1_HSIOM_MASK;
    HSIOM_PORT_SEL2 |=
        (HSIOM_GPIO << RELAY1_HSIOM_SHIFT);

    /* P1_7 -> Strong Drive */
    GPIO_PRT1_PC &= ~RELAY2_DM_MASK;
    GPIO_PRT1_PC |=
        (GPIO_DM_STRONG << RELAY2_DM_SHIFT);

    HSIOM_PORT_SEL1 &= ~RELAY2_HSIOM_MASK;
    HSIOM_PORT_SEL1 |=
        (HSIOM_GPIO << RELAY2_HSIOM_SHIFT);

    /* Both relays OFF initially */
    GPIO_PRT2_DR_CLR = RELAY1_MASK;
    GPIO_PRT1_DR_CLR = RELAY2_MASK;
}

void relay_app_run(void)
{

    /*

     * P1_6:

     * External pull-up

     *

     * 0 = pressed

     */

    if ((GPIO_PRT1_PS & BUTTON1_MASK) == 0U)

    {

        GPIO_PRT2_DR_SET = RELAY1_MASK;

		GPIO_PRT1_DR_SET = RELAY2_MASK;

    }

    else

    {

        GPIO_PRT2_DR_CLR = RELAY1_MASK;

		GPIO_PRT1_DR_CLR = RELAY2_MASK;

    }

}



void relay_app_stop(void)
{
    GPIO_PRT2_DR_CLR = RELAY1_MASK;
    GPIO_PRT1_DR_CLR = RELAY2_MASK;
}


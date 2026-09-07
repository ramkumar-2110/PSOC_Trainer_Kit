/*******************************************************************************
 * File Name: main.c
 *
 * Description:
 * Bare-metal 7-Segment Display driver for PSoC 4.
 *
 * Display Type:
 *     Common Anode
 *
 * Segment Mapping:
 *
 *     A  -> P0_5
 *     B  -> P0_4
 *     C  -> P0_3
 *     D  -> P0_2
 *     E  -> P0_1
 *     F  -> P0_0
 *     G  -> P4_7
 *     DP -> P4_6
 *
 * Common Anode:
 *
 *     GPIO LOW  -> Segment ON
 *     GPIO HIGH -> Segment OFF
 *
 * Operation:
 *
 *     Continuously displays 0 to 9.
 *
 * Bare-metal register-level programming.
 *******************************************************************************/

#include <stdint.h>
#include "seven_segment_app.h"


/*******************************************************************************
 * P0 GPIO REGISTERS
 *******************************************************************************/

#define P0_DR       (*(volatile uint32_t *)0x40040000UL)
#define P0_PS       (*(volatile uint32_t *)0x40040004UL)
#define P0_PC       (*(volatile uint32_t *)0x40040008UL)
#define P0_PC2      (*(volatile uint32_t *)0x40040018UL)

#define P0_DR_SET   (*(volatile uint32_t *)0x40040040UL)
#define P0_DR_CLR   (*(volatile uint32_t *)0x40040044UL)


/*******************************************************************************
 * P4 GPIO REGISTERS
 *******************************************************************************/

#define P4_DR       (*(volatile uint32_t *)0x40040400UL)
#define P4_PS       (*(volatile uint32_t *)0x40040404UL)
#define P4_PC       (*(volatile uint32_t *)0x40040408UL)
#define P4_PC2      (*(volatile uint32_t *)0x40040418UL)

#define P4_DR_SET   (*(volatile uint32_t *)0x40040440UL)
#define P4_DR_CLR   (*(volatile uint32_t *)0x40040444UL)


/*******************************************************************************
 * HSIOM REGISTERS
 *******************************************************************************/

#define HSIOM_PORT_SEL0   (*(volatile uint32_t *)0x40020000UL)
#define HSIOM_PORT_SEL4   (*(volatile uint32_t *)0x40020400UL)


/*******************************************************************************
 * GPIO DRIVE MODE
 *******************************************************************************/

/*
 * 0x06 = Strong Drive
 */

#define GPIO_DM_STRONG    0x06UL


/*******************************************************************************
 * SEGMENT DEFINITIONS
 *******************************************************************************/

/*
 * P0 pins
 */

#define SEG_A_PIN     5U
#define SEG_B_PIN     4U
#define SEG_C_PIN     3U
#define SEG_D_PIN     2U
#define SEG_E_PIN     1U
#define SEG_F_PIN     0U


/*
 * P4 pins
 */

#define SEG_G_PIN     7U
#define SEG_DP_PIN    6U


/*******************************************************************************
 * DISPLAY CONFIGURATION
 *******************************************************************************/

/*
 * Common-anode display:
 *
 * LOW  = ON
 * HIGH = OFF
 */

#define SEG_ON        0U
#define SEG_OFF       1U


/*
 * Decimal point:
 *
 * 0 = OFF
 * 1 = ON
 */

#define DISPLAY_DP    0U


/*
 * Delay between digits.
 */

#define COUNT_DELAY   400U


/*******************************************************************************
 * 7-SEGMENT LOOKUP TABLE
 *
 * Order:
 *
 *     A B C D E F G
 *
 * 1 = segment ON
 * 0 = segment OFF
 *******************************************************************************/

static const uint8_t digitSegments[10][7] =
{
    /*       A  B  C  D  E  F  G */

    /* 0 */ {1, 1, 1, 1, 1, 1, 0},
    /* 1 */ {0, 1, 1, 0, 0, 0, 0},
    /* 2 */ {1, 1, 0, 1, 1, 0, 1},
    /* 3 */ {1, 1, 1, 1, 0, 0, 1},
    /* 4 */ {0, 1, 1, 0, 0, 1, 1},
    /* 5 */ {1, 0, 1, 1, 0, 1, 1},
    /* 6 */ {1, 0, 1, 1, 1, 1, 1},
    /* 7 */ {1, 1, 1, 0, 0, 0, 0},
    /* 8 */ {1, 1, 1, 1, 1, 1, 1},
    /* 9 */ {1, 1, 1, 1, 0, 1, 1}
};


/*******************************************************************************
 * DELAY
 *
 * Approximate millisecond delay.
 *
 * Assumes approximately 48 MHz CPU clock.
 *******************************************************************************/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t i;

    while (ms--)
    {
        for (i = 0; i < 4000U; i++)
        {
            __asm volatile ("nop");
        }
    }
}


/*******************************************************************************
 * GPIO INITIALIZATION
 *******************************************************************************/

static void gpio_init(void)
{
    /***************************************************************************
     * P0_0 -> F
     * P0_1 -> E
     * P0_2 -> D
     * P0_3 -> C
     * P0_4 -> B
     * P0_5 -> A
     *
     * GPIO function = 0x0
     ***************************************************************************/

    HSIOM_PORT_SEL0 &= ~(0x0FUL << 0);
    HSIOM_PORT_SEL0 &= ~(0x0FUL << 4);
    HSIOM_PORT_SEL0 &= ~(0x0FUL << 8);
    HSIOM_PORT_SEL0 &= ~(0x0FUL << 12);
    HSIOM_PORT_SEL0 &= ~(0x0FUL << 16);
    HSIOM_PORT_SEL0 &= ~(0x0FUL << 20);


    /***************************************************************************
     * Configure P0_0 to P0_5 as Strong Drive
     *
     * Each pin occupies 3 bits in P0_PC.
     ***************************************************************************/

    P0_PC &= ~(0x07UL << 0);
    P0_PC |=  (GPIO_DM_STRONG << 0);

    P0_PC &= ~(0x07UL << 3);
    P0_PC |=  (GPIO_DM_STRONG << 3);

    P0_PC &= ~(0x07UL << 6);
    P0_PC |=  (GPIO_DM_STRONG << 6);

    P0_PC &= ~(0x07UL << 9);
    P0_PC |=  (GPIO_DM_STRONG << 9);

    P0_PC &= ~(0x07UL << 12);
    P0_PC |=  (GPIO_DM_STRONG << 12);

    P0_PC &= ~(0x07UL << 15);
    P0_PC |=  (GPIO_DM_STRONG << 15);


    /***************************************************************************
     * Enable input buffers for P0_0 to P0_5.
     ***************************************************************************/

    P0_PC2 &= ~(1UL << 0);
    P0_PC2 &= ~(1UL << 1);
    P0_PC2 &= ~(1UL << 2);
    P0_PC2 &= ~(1UL << 3);
    P0_PC2 &= ~(1UL << 4);
    P0_PC2 &= ~(1UL << 5);


    /***************************************************************************
     * P4_6 -> DP
     * P4_7 -> G
     *
     * GPIO function = 0x0
     ***************************************************************************/

    HSIOM_PORT_SEL4 &= ~(0x0FUL << 24);
    HSIOM_PORT_SEL4 &= ~(0x0FUL << 28);


    /***************************************************************************
     * P4_6 -> Strong Drive
     *
     * P4_6 drive mode -> bits 20:18
     ***************************************************************************/

    P4_PC &= ~(0x07UL << 18);
    P4_PC |=  (GPIO_DM_STRONG << 18);


    /***************************************************************************
     * P4_7 -> Strong Drive
     *
     * P4_7 drive mode -> bits 23:21
     ***************************************************************************/

    P4_PC &= ~(0x07UL << 21);
    P4_PC |=  (GPIO_DM_STRONG << 21);


    /***************************************************************************
     * Enable input buffers.
     ***************************************************************************/

    P4_PC2 &= ~(1UL << 6);
    P4_PC2 &= ~(1UL << 7);


    /***************************************************************************
     * Turn all segments OFF initially.
     *
     * Common Anode:
     * HIGH = OFF
     ***************************************************************************/

    P0_DR_SET = 0x3FUL;

    P4_DR_SET = (1UL << SEG_G_PIN) |
                (1UL << SEG_DP_PIN);
}


/*******************************************************************************
 * DISPLAY DIGIT
 *
 * Displays a single digit from 0 to 9.
 *******************************************************************************/

static void display_digit(uint32_t digit, uint32_t dpOn)
{
    uint32_t mask = 0;


    /***************************************************************************
     * Make sure digit is valid.
     ***************************************************************************/

    if (digit > 9U)
    {
        digit = 0U;
    }


    /***************************************************************************
     * P0 segments:
     *
     * A -> P0_5
     * B -> P0_4
     * C -> P0_3
     * D -> P0_2
     * E -> P0_1
     * F -> P0_0
     *
     * Common Anode:
     *
     * Segment ON  -> LOW
     * Segment OFF -> HIGH
     ***************************************************************************/

    if (digitSegments[digit][0] == 1U)
        mask |= (1UL << SEG_A_PIN);

    if (digitSegments[digit][1] == 1U)
        mask |= (1UL << SEG_B_PIN);

    if (digitSegments[digit][2] == 1U)
        mask |= (1UL << SEG_C_PIN);

    if (digitSegments[digit][3] == 1U)
        mask |= (1UL << SEG_D_PIN);

    if (digitSegments[digit][4] == 1U)
        mask |= (1UL << SEG_E_PIN);

    if (digitSegments[digit][5] == 1U)
        mask |= (1UL << SEG_F_PIN);


    /***************************************************************************
     * Turn OFF all P0 segments first.
     ***************************************************************************/

    P0_DR_SET = 0x3FUL;


    /***************************************************************************
     * Turn ON required P0 segments.
     ***************************************************************************/

    P0_DR_CLR = mask;


    /***************************************************************************
     * G segment.
     *
     * G is connected to P4_7.
     ***************************************************************************/

    if (digitSegments[digit][6] == 1U)
    {
        P4_DR_CLR = (1UL << SEG_G_PIN);
    }
    else
    {
        P4_DR_SET = (1UL << SEG_G_PIN);
    }


    /***************************************************************************
     * Decimal point.
     ***************************************************************************/

    if (dpOn != 0U)
    {
        P4_DR_CLR = (1UL << SEG_DP_PIN);
    }
    else
    {
        P4_DR_SET = (1UL << SEG_DP_PIN);
    }
}

void seven_segment_app_init(void)
{
    gpio_init();
}
void seven_segment_app_run(void)
{
        static uint32_t digit = 0U;
        /***********************************************************************
         * Display current digit.
         ***********************************************************************/

        display_digit(digit, DISPLAY_DP);


        /***********************************************************************
         * Wait 1 second.
         ***********************************************************************/

        delay_ms(COUNT_DELAY);


        /***********************************************************************
         * Next digit.
         ***********************************************************************/

        digit++;

        if (digit > 9U)
        {
            digit = 0U;
        }
}
void seven_segment_app_stop(void)
{
    P0_DR_SET = 0x3FUL;

    P4_DR_SET = (1UL << SEG_G_PIN) |
                (1UL << SEG_DP_PIN);
}
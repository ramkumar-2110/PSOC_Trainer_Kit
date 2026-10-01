/*
 * main.c
 * Description:
 * Bare-metal button-controlled relay operation
 * using direct memory-mapped GPIO register access.
 *
 * Pin Connections:
 * Button 1 -> P1_6
 * Relay 1  -> P2_5
 * Relay 2  -> P1_7
 */

#include <stdint.h>
#include "relay_app.h"

/* ================= PORT 1 ================= */
#define GPIO_PRT1_PS       (*(volatile uint32_t *)0x40040104UL)
#define GPIO_PRT1_PC       (*(volatile uint32_t *)0x40040108UL)
#define GPIO_PRT1_PC2      (*(volatile uint32_t *)0x40040118UL)
#define GPIO_PRT1_DR_SET   (*(volatile uint32_t *)0x40040140UL)
#define GPIO_PRT1_DR_CLR   (*(volatile uint32_t *)0x40040144UL)


/* ================= PORT 2 ================= */
#define GPIO_PRT2_PC       (*(volatile uint32_t *)0x40040208UL)
#define GPIO_PRT2_DR_SET   (*(volatile uint32_t *)0x40040240UL)
#define GPIO_PRT2_DR_CLR   (*(volatile uint32_t *)0x40040244UL)


/* ================= HSIOM ================= */
#define HSIOM_PORT_SEL1    (*(volatile uint32_t *)0x40020100UL)
#define HSIOM_PORT_SEL2    (*(volatile uint32_t *)0x40020200UL)


void relay_app_init(void)
{
    /* Select GPIO function for P2_5 */
    HSIOM_PORT_SEL2 &= ~(0xFUL << 20);

    /* Configure P2_5 as strong-drive output */
    GPIO_PRT2_PC &= ~(0x7UL << 15);
    GPIO_PRT2_PC |=  (0x6UL << 15);

    /* Initialize Relay 1 to OFF */
    GPIO_PRT2_DR_CLR = (1UL << 5);

    /* Select GPIO function for P1_7 */
    HSIOM_PORT_SEL1 &= ~(0xFUL << 28);

    /* Configure P1_7 as strong-drive output */
    GPIO_PRT1_PC &= ~(0x7UL << 21);
    GPIO_PRT1_PC |=  (0x6UL << 21);

    /* Initialize Relay 2 to OFF */
    GPIO_PRT1_DR_CLR = (1UL << 7);

    /* Select GPIO function for P1_6 */
    HSIOM_PORT_SEL1 &= ~(0xFUL << 24);

    /* Configure P1_6 as high-impedance input */
    GPIO_PRT1_PC &= ~(0x7UL << 18);
    GPIO_PRT1_PC |=  (0x1UL << 18);

    /* Enable the input buffer for P1_6 */
    GPIO_PRT1_PC2 &= ~(1UL << 6);
}

void relay_app_run(void)
{
        /*Check if button is pressed (active low)*/
        if ((GPIO_PRT1_PS & (1UL << 6)) == 0U) 
        {
            GPIO_PRT2_DR_SET = (1UL << 5);
            GPIO_PRT1_DR_SET = (1UL << 7);
        }
        else
        {
            GPIO_PRT2_DR_CLR = (1UL << 5);
            GPIO_PRT1_DR_CLR = (1UL << 7);
        }
}

void relay_app_stop(void)
{
    /* Turn off both relays */
    GPIO_PRT2_DR_CLR = (1UL << 5);
    GPIO_PRT1_DR_CLR = (1UL << 7);
}
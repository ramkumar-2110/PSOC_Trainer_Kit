/******************************************************************************
 * File Name:    potentiometer_app.c
 *
 * Description:
 * PSoC Trainer Kit - Potentiometer -> LED Controller
 *
 * Target:
 * CY8C4147AZI-S475
 *
 * Potentiometer:
 *     P2_1
 *
 * LEDs:
 *     LED1 -> P3_5
 *     LED2 -> P3_3
 *     LED3 -> P2_7
 *
 * ADC:
 *     SAR0 Channel 0
 *     P2_1 -> AMUXA -> SAR0 VPLUS0
 *
 * No PDL
 * No HAL
 ******************************************************************************/

#include <stdint.h>
#include "potentiometer_app.h"


/******************************************************************************
 * GPIO REGISTERS
 ******************************************************************************/

#define P2_PC           (*(volatile uint32_t *)0x40040208UL)
#define P2_PC2          (*(volatile uint32_t *)0x40040218UL)

#define P2_DR_SET       (*(volatile uint32_t *)0x40040240UL)
#define P2_DR_CLR       (*(volatile uint32_t *)0x40040244UL)

#define P3_PC           (*(volatile uint32_t *)0x40040308UL)
#define P3_PC2          (*(volatile uint32_t *)0x40040318UL)

#define P3_DR_SET       (*(volatile uint32_t *)0x40040340UL)
#define P3_DR_CLR       (*(volatile uint32_t *)0x40040344UL)


/******************************************************************************
 * HSIOM REGISTERS
 ******************************************************************************/

#define HSIOM_PORT_SEL2     (*(volatile uint32_t *)0x40020200UL)
#define HSIOM_PORT_SEL3     (*(volatile uint32_t *)0x40020300UL)


/******************************************************************************
 * PERIPHERAL CLOCK REGISTERS
 ******************************************************************************/

#define PERI_DIV_CMD        (*(volatile uint32_t *)0x40010000UL)

#define PERI_DIV_8_CTL0     (*(volatile uint32_t *)0x40010200UL)

#define PERI_DIV_16_CTL0    (*(volatile uint32_t *)0x40010300UL)


/******************************************************************************
 * SAR ADC REGISTERS
 ******************************************************************************/

#define SAR_CTRL                (*(volatile uint32_t *)0x403A0000UL)

#define SAR_SAMPLE_CTRL         (*(volatile uint32_t *)0x403A0004UL)

#define SAR_SAMPLE_TIME01       (*(volatile uint32_t *)0x403A0010UL)

#define SAR_CHAN_EN             (*(volatile uint32_t *)0x403A0020UL)

#define SAR_START_CTRL          (*(volatile uint32_t *)0x403A0024UL)

#define SAR_CHAN_CONFIG0        (*(volatile uint32_t *)0x403A0080UL)

#define SAR_CHAN_RESULT0        (*(volatile uint32_t *)0x403A0180UL)

#define SAR_CHAN_RESULT_VALID   (*(volatile uint32_t *)0x403A0204UL)

#define SAR_MUX_SWITCH0         (*(volatile uint32_t *)0x403A0300UL)

/*
 * This is the CLEAR register corresponding to SAR_MUX_SWITCH0.
 */
#define SAR_MUX_SWITCH_CLEAR0   (*(volatile uint32_t *)0x403A0304UL)

#define SAR_MUX_SWITCH_HW       (*(volatile uint32_t *)0x403A0340UL)


/******************************************************************************
 * SYSTICK REGISTERS
 ******************************************************************************/

#define SYST_CSR                (*(volatile uint32_t *)0xE000E010UL)

#define SYST_RVR                (*(volatile uint32_t *)0xE000E014UL)

#define SYST_CVR                (*(volatile uint32_t *)0xE000E018UL)


/******************************************************************************
 * CLOCK
 ******************************************************************************/

#define IMO_HZ                  24000000UL


/******************************************************************************
 * PIN DEFINITIONS
 ******************************************************************************/

#define POT_PIN                 1U

#define LED1_PIN                5U

#define LED2_PIN                3U

#define LED3_PIN                7U


/******************************************************************************
 * ADC DEFINITIONS
 ******************************************************************************/

#define ADC_MAX_VALUE           4095UL


/******************************************************************************
 * DELAY FUNCTION
 ******************************************************************************/

static void delay_us(uint32_t us)
{
    uint32_t ticks;

    if (us == 0U)
    {
        return;
    }

    ticks = (IMO_HZ / 1000000UL) * us;

    if (ticks > 0x00FFFFFFUL)
    {
        ticks = 0x00FFFFFFUL;
    }

    SYST_RVR = ticks - 1U;

    SYST_CVR = 0U;

    SYST_CSR = 0x05U;

    while ((SYST_CSR & (1UL << 16U)) == 0UL)
    {
    }

    SYST_CSR = 0U;
}


/******************************************************************************
 * LED INITIALIZATION
 ******************************************************************************/

static void led_init(void)
{
    /**************************************************************************
     * LED1 -> P3_5
     **************************************************************************/

    HSIOM_PORT_SEL3 &= ~(0x0FUL << 20U);

    P3_PC &= ~(0x07UL << 15U);
    P3_PC |= (6UL << 15U);

    P3_PC2 &= ~(1UL << LED1_PIN);

    P3_DR_CLR = (1UL << LED1_PIN);


    /**************************************************************************
     * LED2 -> P3_3
     **************************************************************************/

    HSIOM_PORT_SEL3 &= ~(0x0FUL << 12U);

    P3_PC &= ~(0x07UL << 9U);
    P3_PC |= (6UL << 9U);

    P3_PC2 &= ~(1UL << LED2_PIN);

    P3_DR_CLR = (1UL << LED2_PIN);


    /**************************************************************************
     * LED3 -> P2_7
     **************************************************************************/

    HSIOM_PORT_SEL2 &= ~(0x0FUL << 28U);

    P2_PC &= ~(0x07UL << 21U);
    P2_PC |= (6UL << 21U);

    P2_PC2 &= ~(1UL << LED3_PIN);

    P2_DR_CLR = (1UL << LED3_PIN);
}


/******************************************************************************
 * ALL LEDs OFF
 ******************************************************************************/

static void leds_off(void)
{
    P3_DR_CLR = (1UL << LED1_PIN);

    P3_DR_CLR = (1UL << LED2_PIN);

    P2_DR_CLR = (1UL << LED3_PIN);
}


/******************************************************************************
 * ADC INITIALIZATION
 ******************************************************************************/

static void adc_init(void)
{
    /**************************************************************************
     * 1. Stop any previous SAR conversion
     **************************************************************************/

    SAR_START_CTRL = 0U;

    SAR_CHAN_EN = 0U;


    /**************************************************************************
     * 2. Clear previous firmware SARMUX connections
     *
     * P2_1 = bit 1
     * P2_2 = bit 2
     **************************************************************************/

    SAR_MUX_SWITCH_CLEAR0 = 0xFFFFFFFFUL;


    /**************************************************************************
     * 3. Clear channel configuration
     **************************************************************************/

    SAR_CHAN_CONFIG0 = 0U;


    /**************************************************************************
     * 4. Configure P2_1 as analog input
     *
     * P2_1 -> AMUXA
     **************************************************************************/

    HSIOM_PORT_SEL2 &= ~(0x0FUL << 4U);

    HSIOM_PORT_SEL2 |= (6UL << 4U);

    /*
     * Analog High-Z
     */
    P2_PC &= ~(0x07UL << 3U);

    /*
     * Disable digital input buffer
     */
    P2_PC2 |= (1UL << POT_PIN);


    /**************************************************************************
     * 5. Configure SAR clock
     *
     * IMO = 24 MHz
     * SAR clock = 24 MHz / 24 = 1 MHz
     **************************************************************************/

    PERI_DIV_16_CTL0 = (23UL << 8U);

    PERI_DIV_CMD =
          (1UL << 31U)
        | (0x3UL << 14U)
        | (0x3FUL << 8U)
        | (1UL << 6U);


    PERI_DIV_8_CTL0 = (23UL << 8U);

    PERI_DIV_CMD =
          (1UL << 31U)
        | (0x3UL << 14U)
        | (0x3FUL << 8U)
        | (1UL << 6U);


    /**************************************************************************
     * 6. Route SAR clock
     *
     * Do NOT configure all 64 PCLK slots.
     **************************************************************************/

    (*(volatile uint32_t *)0x40010148UL) = (1UL << 6U);


    /**************************************************************************
     * 7. Configure SAR
     **************************************************************************/

    SAR_CTRL =
          (1UL << 31U)
        | (7UL << 4U)
        | (0UL << 9U);


    delay_us(20U);


    /**************************************************************************
     * 8. Sample time
     **************************************************************************/

    SAR_SAMPLE_TIME01 = 128UL;


    /**************************************************************************
     * 9. Continuous conversion
     **************************************************************************/

    SAR_SAMPLE_CTRL = (1UL << 16U);


    /**************************************************************************
     * 10. Configure channel 0
     *
     * P2_1 -> VPLUS0
     **************************************************************************/

    SAR_CHAN_CONFIG0 = 1UL;


    /**************************************************************************
     * 11. Enable channel 0
     **************************************************************************/

    SAR_CHAN_EN = (1UL << 0U);


    /**************************************************************************
     * 12. Connect P2_1 to SARMUX
     **************************************************************************/

    SAR_MUX_SWITCH_HW = (1UL << 1U);

    SAR_MUX_SWITCH0 = (1UL << 1U);


    /**************************************************************************
     * 13. Start conversion
     **************************************************************************/

    SAR_START_CTRL = 1UL;
}


/******************************************************************************
 * ADC READ
 ******************************************************************************/

static uint32_t adc_read(void)
{
    uint32_t raw;

    /*
     * Wait until channel 0 result is valid.
     */
    while ((SAR_CHAN_RESULT_VALID & (1UL << 0U)) == 0UL)
    {
    }

    raw = SAR_CHAN_RESULT0 & 0x0FFFUL;

    /*
     * Convert signed ADC result to 0-4095.
     */
    raw ^= 0x800UL;

    return raw;
}


/******************************************************************************
 * LED CONTROL
 ******************************************************************************/

static void led_control(uint32_t adc_value)
{
    /*
     * Turn all LEDs OFF first.
     */
    leds_off();


    /*
     * 0% - 33%
     *
     * LED1 ON
     */
    if ((adc_value < 1365UL) && (adc_value > 500UL))
    {
        P3_DR_SET = (1UL << LED1_PIN);
    }


    /*
     * 33% - 66%
     *
     * LED1 + LED2 ON
     */
    else if ((adc_value < 2730UL) && (adc_value > 1365UL))
    {
        P3_DR_SET = (1UL << LED1_PIN);
        P3_DR_SET = (1UL << LED2_PIN);
    }


    /*
     * 66% - 100%
     *
     * LED1 + LED2 + LED3 ON
     */
    else if (adc_value > 2730UL)
    {
        P3_DR_SET = (1UL << LED1_PIN);
        P3_DR_SET = (1UL << LED2_PIN);
        P2_DR_SET = (1UL << LED3_PIN);
    }
}


/******************************************************************************
 * APPLICATION INIT
 ******************************************************************************/

void potentiometer_app_init(void)
{
    led_init();

    adc_init();

    leds_off();
}


/******************************************************************************
 * APPLICATION RUN
 ******************************************************************************/

void potentiometer_app_run(void)
{
    uint32_t adc_value;

    adc_value = adc_read();

    if (adc_value > ADC_MAX_VALUE)
    {
        adc_value = ADC_MAX_VALUE;
    }

    led_control(adc_value);

    delay_us(1000U);
}


/******************************************************************************
 * APPLICATION STOP
 ******************************************************************************/

void potentiometer_app_stop(void)
{
    /*
     * Turn LEDs OFF.
     *
     * SAR is stopped/reconfigured by the next ADC application.
     */
    leds_off();
}
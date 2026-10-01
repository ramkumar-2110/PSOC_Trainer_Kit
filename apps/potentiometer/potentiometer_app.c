/*
Description: ADC code to read the voltage level from the potentiometer
LED is connected with P3.5
Potentiometer is connected with P2.1
LED will glow once the volatage level become greater than the threshold
*/

#include<stdint.h>
#include "potentiometer_app.h"

/*PORT 3*/
#define HSIOM_PORT_SEL3  (*(volatile uint32_t*) 0x40020300)
#define GPIO_PRT3_DR     (*(volatile uint32_t*) 0x40040300)
#define GPIO_PRT3_PS     (*(volatile uint32_t*) 0x40040304)
#define GPIO_PRT3_PC     (*(volatile uint32_t*) 0x40040308)
#define GPIO_PRT3_DR_SET (*(volatile uint32_t*) 0x40040340)
#define GPIO_PRT3_DR_CLR (*(volatile uint32_t*) 0x40040344)
#define GPIO_PRT3_DR_INV (*(volatile uint32_t*) 0x40040348)

/*PORT 2*/
#define HSIOM_PORT_SEL2  (*(volatile uint32_t*) 0x40020200)
#define GPIO_PRT2_PC     (*(volatile uint32_t*) 0x40040208)
#define GPIO_PRT2_PC2    (*(volatile uint32_t*) 0x40040218)

/*SAR ADC*/
#define SAR_CTRL               (*(volatile uint32_t*) 0x403A0000)
#define SAR_MUX_SWITCH0        (*(volatile uint32_t*) 0x403A0300)
#define SAR_MUX_SWITCH_HW_CTRL (*(volatile uint32_t*) 0x403A0340)
#define SAR_MUX_SWITCH_CLEAR0  (*(volatile uint32_t*) 0x403A0304)
#define SAR_SAMPLE_CTRL        (*(volatile uint32_t*) 0x403A0004)
#define SAR_SAMPLE_TIME01      (*(volatile uint32_t*) 0x403A0010)
#define SAR_CHAN_CONFIG0       (*(volatile uint32_t*) 0x403A0080)
#define SAR_CHAN_EN            (*(volatile uint32_t*) 0x403A0020)
#define SAR_START_CTRL         (*(volatile uint32_t*) 0x403A0024)
#define SAR_INTR               (*(volatile uint32_t*) 0x403A0210)
#define SAR_CHAN_RESULT0       (*(volatile uint32_t*) 0x403A0180)


/*Clock registers*/
#define PERI_DIV_CMD           (*(volatile uint32_t*) 0x40010000)
#define PERI_DIV_8_CTL0        (*(volatile uint32_t*) 0x40010200)
#define PERI_DIV_16_CTL0       (*(volatile uint32_t*) 0x40010300)

void clock_config(void)
{
    /*Configure SAR clock = 24 MHz / 24 = 1 MHz*/
    PERI_DIV_16_CTL0 = (23UL << 8U); 
    PERI_DIV_CMD = (1UL << 31U) | (0x3UL << 14U) | (0x3FUL << 8U) | (1UL << 6U);
    PERI_DIV_8_CTL0 = (23UL << 8U);
    PERI_DIV_CMD = (1UL << 31U) | (0x3UL << 14U) | (0x3FUL << 8U);

    (*(volatile uint32_t *)0x40010148UL) = (1UL << 6U); //Route divider to peripheral clock slots.
 }

volatile uint16_t chanresult = 0;

void potentiometer_app_init(void)
{
    clock_config();
    /*led configurations*/
    HSIOM_PORT_SEL3 = 0x00000000; //clear previous configuration and set GPIO function for all pins of port 3
    GPIO_PRT3_PC &= (~(0x7 << 15)); //clear previous configuration
    GPIO_PRT3_PC |= (0x6 << 15); //set pin 3.5 as strong drive output (3 bits used to select drive mode so for pin 4--> 5*3=15    
    GPIO_PRT3_DR = 0x00000000; //set initial value as LOW

    GPIO_PRT3_PC &= (~(0x7 << 12));
    GPIO_PRT3_PC |= (0x6 << 12);
    GPIO_PRT3_DR = 0x00000000;


    /*adc pin configurations*/
    HSIOM_PORT_SEL2 = 0x00000000;//clear previous configurations
    HSIOM_PORT_SEL2 |= (0x6 << 4);//configures P2_1 for analog mode
    GPIO_PRT2_PC &= (~(0x7 << 3));//clear previous configuration and set the pin to anolog mode with high impedence(z)
    GPIO_PRT2_PC2 &= (~(0x1 << 1));//clear previous configuration
    GPIO_PRT2_PC2 |= (0x1 << 1); //disables input buffer

    /*adc configurations*/
    SAR_CTRL = 0x00000000; //clear previous configurations
    SAR_CTRL |= (0x7 << 4); //set Vref to VDDA
    SAR_CTRL |= (0x1 << 31); //enables sar
    SAR_CTRL |= (0x7 << 9); //set NEG_SEL to Vref
                /* SAR_CTRL bits value selection 
                NEG_SEL (Vneg) default value is 0 for single ended mode [11:9]
                VREF_BYP_CAP_EN default value is 0 which is correct value when VREF buffer is OFF [7]
                SAR_HW_CTRL_NEGVREF default 0 used for only firmware control [13]
                PWR_CTRL_VREF default value 0 used for normal power for Vref buffer at 18MHz SAR frequency [15:14]
                ICONT_LV default value 0 used for normal power mode in ADC [25:24]
                DEEPSLEEP_ON default value 0 indicates deepsleep is OFF [27]
                */
    SAR_MUX_SWITCH0 = 0x00000000; //clear previous configurations
    SAR_MUX_SWITCH0 |= (0x1 << 16); //close switch between Vssa and Vminus signal
    SAR_MUX_SWITCH0 |= (0x1 << 1); //close switch between P2_0 and Vplus signal
    SAR_SAMPLE_CTRL = 0x00000000; //clear previous configurations   
    SAR_SAMPLE_CTRL |= (0x1 << 16); //enables continuous mode
    SAR_SAMPLE_TIME01 = 0x00060006; //aquisition time
    SAR_CHAN_CONFIG0 = 0x00000001; //clear previous configuration and selects the SAR MUX in port addr and pin 1
    SAR_CHAN_EN = 0x00000000;
    SAR_CHAN_EN = (1 << 0); //enables channel 0
    SAR_START_CTRL = 0x00000000; //clear previous configurations
    
    
    SAR_START_CTRL |= (0x1 << 0); //start adc conversion
}

void potentiometer_app_run(void)
{
        chanresult = SAR_CHAN_RESULT0 & 0x0FFF;

        if(chanresult >2048) //turn on LED if the voltage level is greater than the threshold
        {
            GPIO_PRT3_DR_SET = (0x1 << 5);
        }
        else
        {
            GPIO_PRT3_DR_CLR = (0x1 << 5);
        }
}

void potentiometer_app_stop(void)
{
    SAR_CHAN_EN = 0x00000000; //disables channel 0
    SAR_CTRL &= (~(0x1 << 31)); //disables sar
    GPIO_PRT3_DR_CLR = (0x1 << 5); //turn off LED
}
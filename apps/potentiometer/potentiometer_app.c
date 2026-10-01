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
#define GPIO_PRT2_DR     (*(volatile uint32_t*) 0x40040200)
#define GPIO_PRT2_DR_SET (*(volatile uint32_t*) 0x40040240)
#define GPIO_PRT2_DR_CLR (*(volatile uint32_t*) 0x40040244)
#define GPIO_PRT2_DR_INV (*(volatile uint32_t*) 0x40040248)

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
        /**********************LED configurations*******************/
    /*configure in GPIO mode*/
    HSIOM_PORT_SEL3 &= (~(0xF << 23)); 
    HSIOM_PORT_SEL2 &= (~(0xF << 28));

    /*Configure GPIO in Strong drive for output*/
    GPIO_PRT3_PC &= (~(0x7 << 15)); 
    GPIO_PRT3_PC |= (0x6 << 15); 
    GPIO_PRT2_PC &= (~(0x7 << 21)); 
    GPIO_PRT2_PC |= (0x6 << 21);

    /*set initial value as LOW for led*/
    GPIO_PRT3_DR = 0x00000000;
    GPIO_PRT2_DR = 0x00000000;


    /*********************adc pin configurations******************/
    /*configures P2_1 for analog mode*/
    HSIOM_PORT_SEL2 = 0x00000000;
    HSIOM_PORT_SEL2 |= (0x6 << 4);

    /*Configure ADC pin drive with high impedence*/
    GPIO_PRT2_PC &= (~(0x7 << 3));

    /*Disable input buffer*/
    GPIO_PRT2_PC2 &= (~(0x1 << 1));
    GPIO_PRT2_PC2 |= (0x1 << 1);

    /*********************adc configurations***********************/
    /*Vref=VDDA , NEG_SEL=Vref , Enable SAR*/
    SAR_CTRL = 0x00000000;
    SAR_CTRL |= ((0x7 << 4) | (0x7 << 9) | (0x1 << 31));

    /*close switch between Vssa and Vminus ,P2_0 and Vplus signal*/
    SAR_MUX_SWITCH0 = 0x00000000;
    SAR_MUX_SWITCH0 |= ((0x1 << 16) | (0x1 << 1));

    /*Enables continous mode*/
    SAR_SAMPLE_CTRL = 0x00000000;
    SAR_SAMPLE_CTRL |= (0x1 << 16);

    /*Configure acquisition time as 6 clock cycles*/
    SAR_SAMPLE_TIME01 = 0x00060006;

    /*configure channel with SARmux port addr and pin 1*/
    SAR_CHAN_CONFIG0 = 0x00000001;

    /*Enables channel 0*/
    SAR_CHAN_EN = 0x00000000;
    SAR_CHAN_EN |= (0x1 << 0); 
    
    /*Start adc conversion*/
    SAR_START_CTRL = 0x00000000;
    SAR_START_CTRL |= (0x1 << 0);
}

void potentiometer_app_run(void)
{
        chanresult = SAR_CHAN_RESULT0 & 0x0FFF;

        if(chanresult >1365 && chanresult < 2730) //turn on LED if the voltage level is greater than the threshold
        {
            GPIO_PRT3_DR_SET = (0x1 << 5);
            GPIO_PRT2_DR_CLR = (0x1 << 7);
        }
        else if (chanresult > 2730) //turn off LED if the voltage level is less than the threshold
        {
            GPIO_PRT2_DR_SET = (0x1 << 7);
            GPIO_PRT3_DR_SET = (0x1 << 5);
        }
        else //turn off LED if the voltage level is less than the threshold
        {
            GPIO_PRT3_DR_CLR = (0x1 << 5);
            GPIO_PRT2_DR_CLR = (0x1 << 7);
        }
}

void potentiometer_app_stop(void)
{
    SAR_CHAN_EN = 0x00000000; //disables channel 0
    GPIO_PRT3_DR_CLR = (0x1 << 5); //turn off LED
    GPIO_PRT2_DR_CLR = (0x1 << 7); //turn off LED
    HSIOM_PORT_SEL2 = 0x00000000;//open switch between AMUXA and P2_1
    SAR_MUX_SWITCH_CLEAR0 = 0xffffffff; //clear previous configurations
}
#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg_peripherals.h"
#include "cycfg_pins.h"

#include "eeprom_app.h"
#include "m95m04.h"

/* ---------------------------------------------------------
 * SPI context
 * --------------------------------------------------------- */
static cy_stc_scb_spi_context_t spi_context;

/* ---------------------------------------------------------
 * Application state
 * --------------------------------------------------------- */
static bool eeprom_initialized = false;


/* ---------------------------------------------------------
 * EEPROM APPLICATION INITIALIZATION
 * --------------------------------------------------------- */
void eeprom_app_init(void)
{
    eeprom_initialized = false;

    /* -----------------------------------------------------
     * Initialize SPI
     *
     * SCB0
     * Device Configurator:
     * SPI Master
     * Motorola
     * Mode 0
     * 8-bit
     * MSB first
     * ----------------------------------------------------- */

    if (Cy_SCB_SPI_Init(
            SCB0,
            &scb_0_config,
            &spi_context) != CY_SCB_SPI_SUCCESS)
    {
        return;
    }

    Cy_SCB_SPI_Enable(SCB0);

    /* -----------------------------------------------------
     * Initialize EEPROM CS
     *
     * EEPROM CS = P1[3]
     * ----------------------------------------------------- */

    m95m04_init();

    /* -----------------------------------------------------
     * Run EEPROM test
     * ----------------------------------------------------- */

    m95m04_test();

    eeprom_initialized = true;

    /* -----------------------------------------------------
     * Trainer Kit success message
     * ----------------------------------------------------- */

    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: EEPROM\r\n"
    );
}


/* ---------------------------------------------------------
 * EEPROM APPLICATION RUN
 * --------------------------------------------------------- */
void eeprom_app_run(void)
{
    /*
     * m95m04_test() is already the complete EEPROM test
     * and has been executed during initialization.
     *
     * Nothing needs to run continuously.
     */
    if (!eeprom_initialized)
    {
        return;
    }
}


/* ---------------------------------------------------------
 * EEPROM APPLICATION STOP
 * --------------------------------------------------------- */
void eeprom_app_stop(void)
{
    eeprom_initialized = false;

    /*
     * m95m04_init() controls the EEPROM CS pin.
     * Leave the EEPROM deselected when stopping the app.
     *
     * If your m95m04 driver provides a dedicated CS-high
     * function, it can be called here.
     */
}
#include "cybsp.h"
#include "cycfg_peripherals.h"
#include "command_parser.h"
#include "app_manager.h"

cy_stc_scb_i2c_context_t i2c_context;

int main(void)
{
    cy_rslt_t result;
cy_en_scb_i2c_status_t i2c_status;

    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    /* Initialize Device Configurator peripherals */
    init_cycfg_all();

    /* Initialize I2C PHR on SCB1 */
    i2c_status = Cy_SCB_I2C_Init(
    I2C_PHR_HW,
    &I2C_PHR_config,
    &i2c_context
);



    Cy_SCB_I2C_Enable(
        I2C_PHR_HW,
        &i2c_context
    );

    /* Initialize UART_KP on SCB3 */
    Cy_SCB_UART_Init(
        SCB3,
        &UART_KP_config,
        NULL
    );

    Cy_SCB_UART_Enable(SCB3);

if (i2c_status == CY_SCB_I2C_SUCCESS)
{
    Cy_SCB_UART_PutString(
        SCB3,
        "I2C INIT OK\r\n"
    );
}
else
{
    Cy_SCB_UART_PutString(
        SCB3,
        "I2C INIT FAILED\r\n"
    );
}

    app_manager_init();
    command_parser_init();

    Cy_SCB_UART_PutString(
        SCB3,
        "\r\n"
        "================================\r\n"
        "       PSoC TRAINER KIT\r\n"
        "================================\r\n"
        "Command interface ready\r\n"
        "================================\r\n"
    );

    while (1)
    {
        command_parser_process();
        app_manager_run();
    }
}
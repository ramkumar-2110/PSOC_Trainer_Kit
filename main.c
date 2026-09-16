#include "cybsp.h"
#include "cycfg_peripherals.h"
#include "command_parser.h"
#include "app_manager.h"

cy_stc_scb_i2c_context_t i2c_context;
cy_stc_scb_spi_context_t spi_context;

static void process_esp_uart(void)
{
    static char buffer[8];
    static uint32_t index = 0;

    while (Cy_SCB_UART_GetNumInRxFifo(wifi_uart_HW) > 0)
    {
        char c = (char)Cy_SCB_UART_Get(wifi_uart_HW);

        /* Ignore carriage return */
        if (c == '\r')
        {
            continue;
        }

        /* End of command */
        if (c == '\n')
        {
            if (index > 0)
            {
                uint32_t command = 0;
                bool valid = true;

                buffer[index] = '\0';

                /* Convert ASCII number to integer */
                for (uint32_t i = 0; i < index; i++)
                {
                    if ((buffer[i] < '0') ||
                        (buffer[i] > '9'))
                    {
                        valid = false;
                        break;
                    }

                    command =
                        (command * 10UL) +
                        (uint32_t)(buffer[i] - '0');
                }

                /* Process valid command */
                if (valid)
                {
                    /* 0 = STOP */
                    if (command == 0)
                    {
                        app_manager_stop();
                    }

                    /* 1 to 19 = applications */
                    else if ((command >= 1) &&
                             (command <= 18))
                    {
                        app_manager_start(
                            (app_id_t)command
                        );
                    }
                }

                /* Reset receive buffer */
                index = 0;
            }
        }

        /* Store received character */
        else
        {
            if (index < (sizeof(buffer) - 1U))
            {
                buffer[index++] = c;
            }
            else
            {
                /* Buffer overflow -> discard command */
                index = 0;
            }
        }
    }
}

int main(void)
{
    cy_rslt_t result;
cy_en_scb_i2c_status_t i2c_status;
cy_en_scb_spi_status_t spi_status;

    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    /* Initialize Device Configurator peripherals */
    init_cycfg_all();

spi_status = Cy_SCB_SPI_Init(
    SCB0,
    &scb_0_config,
    &spi_context
);

if (spi_status == CY_SCB_SPI_SUCCESS)
{
    Cy_SCB_SPI_SetActiveSlaveSelect(
        SCB0,
        CY_SCB_SPI_SLAVE_SELECT2
    );

    Cy_SCB_SPI_Enable(SCB0);
}

//Cy_SCB_SPI_Enable(SCB0);

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

    Cy_SCB_UART_Init(
        wifi_uart_HW,
        &wifi_uart_config,
        NULL
    );

    Cy_SCB_UART_Enable(wifi_uart_HW);

    Cy_SCB_UART_PutString(
        wifi_uart_HW,
        "PSOC_READY\r\n"
    );


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
        process_esp_uart();
        app_manager_run();
    }
}
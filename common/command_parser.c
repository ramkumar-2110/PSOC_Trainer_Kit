#include "command_parser.h"

#include "cycfg_peripherals.h"
#include "app_manager.h"
#include <stdlib.h>

#define COMMAND_BUFFER_SIZE 8U

static char command_buffer[COMMAND_BUFFER_SIZE];
static uint32_t command_index = 0U;


/* ============================================================
 * INITIALIZE COMMAND PARSER
 * ============================================================ */

void command_parser_init(void)
{
    command_index = 0U;
}


/* ============================================================
 * COMMAND PARSER
 * ============================================================ */

void command_parser_process(void)
{
    while (Cy_SCB_UART_GetNumInRxFifo(SCB3) > 0U)
    {
        char received =
            (char)Cy_SCB_UART_Get(SCB3);

        if (received == '\r' || received == '\n')
        {
            if (command_index > 0U)
            {
                command_buffer[command_index] = '\0';

                int command =
                    atoi(command_buffer);

                switch (command)
                {
                    case 0:
                        app_manager_stop();

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: STOP\r\n"
                        );
                        break;

                    case 1:
                        app_manager_start(APP_LED);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: LED\r\n"
                        );
                        break;

                    case 2:
                        app_manager_start(APP_BUTTON);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: BUTTON\r\n"
                        );
                        break;
			
		            case 3:
                        app_manager_start(APP_RELAY);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: RELAY\r\n"
                        );
                        break;

                    case 4:
                        app_manager_start(APP_TOUCH);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: TOUCH\r\n"
                        );
                        break;

                    case 6:
                        app_manager_start(APP_PIR);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: PIR\r\n"
                        );
                        break;

                    case 7:
                        app_manager_start(APP_7_SEGMENT); 
                          
                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: 7-SEGMENT\r\n"
                        );
                        break;

                    case 8:
                        app_manager_start(APP_JOYSTICK);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: JOYSTICK\r\n"
                        );
                        break;

                    case 9:
                        app_manager_start(APP_SERVO);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: SERVO\r\n"
                        );
                        break;

                    case 10:
                        app_manager_start(APP_POTENTIOMETER);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: POTENTIOMETER\r\n"
                        );
                        break;

                    case 11:
                        app_manager_start(APP_IO_EXPANDER);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: IO EXPANDER\r\n"
                        );
                        break;

                    case 12:
                        app_manager_start(APP_PROXIMITY);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: PROXIMITY\r\n"
                        );
                        break;

                    case 13:
                        app_manager_start(APP_IMU);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: IMU\r\n"
                        );
                        break;

                    case 14:
                        app_manager_start(APP_TEMPERATURE);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: TEMPERATURE\r\n"
                        );
                        break;

                    case 15:
                        app_manager_start(APP_OLED);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: OLED\r\n"
                        );
                        break;

                    case 16:
                        app_manager_start(APP_SECURE);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: SECURE\r\n"
                        );
                        break;

                    case 17:
                        app_manager_start(APP_EEPROM);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: EEPROM\r\n"
                        );
                        break;

                    case 18:
                        app_manager_start(APP_SD_CARD);

                        Cy_SCB_UART_PutString(
                            SCB3,
                            "OK: SD CARD\r\n"
                        );
                        break;

                    default:
                        Cy_SCB_UART_PutString(
                            SCB3,
                            "ERR: UNKNOWN COMMAND\r\n"
                        );
                        break;
                }

                command_index = 0U;
            }

            continue;
        }

        if (command_index <
            COMMAND_BUFFER_SIZE - 1U)
        {
            command_buffer[command_index++] =
                received;
        }
        else
        {
            command_index = 0U;

            Cy_SCB_UART_PutString(
                SCB3,
                "ERR: COMMAND TOO LONG\r\n"
            );
        }
    }
}

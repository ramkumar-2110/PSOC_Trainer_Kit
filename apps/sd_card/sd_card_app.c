#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg_peripherals.h"
#include "cycfg_pins.h"

#include "ff.h"
#include "fat_fs.h"

#include "sd_card_app.h"

#include <stdio.h>
#include <string.h>


static bool sd_card_initialized = false;


/* =========================================================
 * FAT timestamp
 * ========================================================= */

DWORD get_fattime(void)
{
    /*
     * FAT timestamp:
     *
     * Date:
     *   2026-09-09
     *
     * Time:
     *   10:00:00
     */

    DWORD fattime = 0;

    /* Year: 2026 - 1980 = 46 */
    fattime |= ((DWORD)(2026 - 1980) << 25);

    /* Month: September = 9 */
    fattime |= ((DWORD)9 << 21);

    /* Day: 9 */
    fattime |= ((DWORD)9 << 16);

    /* Hour: 10 */
    fattime |= ((DWORD)10 << 11);

    /* Minute: 0 */
    fattime |= ((DWORD)0 << 5);

    /* Seconds / 2: 0 */
    fattime |= 0;

    return fattime;
}


/* =========================================================
 * SD CARD APPLICATION INITIALIZATION
 * ========================================================= */

void sd_card_app_init(void)
{
    const char test_text[] =
        "Hello from PSoC 4 SD Card!\r\n"
        "FatFs SPI test successful.\r\n";

    char read_buffer[128];


    sd_card_initialized = false;


    /* =====================================================
     * SD + FATFS
     * ===================================================== */

    Cy_SCB_UART_PutString(
        SCB3,
        "[SD Card] Initializing SPI interface "
        "(MOSI=P1_0, MISO=P1_1, SCK=P1_2, CS=P1_4)...\r\n"
    );


    /* =====================================================
     * MOUNT
     * ===================================================== */

    if (!fat_fs_mount())
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[FATFS Error] Failed to mount volume!\r\n"
        );

        return;
    }


    Cy_SCB_UART_PutString(
        SCB3,
        "[FATFS] Volume mounted successfully.\r\n"
    );


    /* =====================================================
     * CREATE FOLDER
     * ===================================================== */

    Cy_SCB_UART_PutString(
        SCB3,
        "[FATFS] Creating folder 'MY_PROJ'...\r\n"
    );


    if (!fat_fs_mkdir("MY_PROJ"))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[FATFS Error] Failed to create folder!\r\n"
        );

        return;
    }
    else
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[FATFS] Folder ready.\r\n"
        );
    }


    /* =====================================================
     * WRITE
     * ===================================================== */

    Cy_SCB_UART_PutString(
        SCB3,
        "[FATFS] Writing to file "
        "'MY_PROJ/EXAMPLE.TXT'...\r\n"
    );


    if (!fat_fs_write_file(
            "MY_PROJ",
            "EXAMPLE.TXT",
            test_text))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[FATFS Error] Failed to write file!\r\n"
        );

        return;
    }


    Cy_SCB_UART_PutString(
        SCB3,
        "[FATFS] File written successfully.\r\n"
    );


    /* =====================================================
     * READ
     * ===================================================== */

    Cy_SCB_UART_PutString(
        SCB3,
        "[FATFS] Reading file "
        "'MY_PROJ/EXAMPLE.TXT' back...\r\n"
    );


    if (!fat_fs_read_file(
            "MY_PROJ",
            "EXAMPLE.TXT",
            read_buffer,
            sizeof(read_buffer)))
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[FATFS Error] Failed to read file back!\r\n"
        );

        return;
    }


    /* =====================================================
     * DISPLAY READ DATA
     * ===================================================== */

    Cy_SCB_UART_PutString(
        SCB3,
        "[FATFS] Read successful!\r\n"
    );


    Cy_SCB_UART_PutString(
        SCB3,
        "--------------------------------------------------\r\n"
    );


    Cy_SCB_UART_PutString(
        SCB3,
        "Read data:\r\n"
    );


    Cy_SCB_UART_PutString(
        SCB3,
        read_buffer
    );


    Cy_SCB_UART_PutString(
        SCB3,
        "--------------------------------------------------\r\n"
    );


    /* =====================================================
     * VERIFY
     * ===================================================== */

    if (strcmp(read_buffer, test_text) == 0)
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[FATFS] READBACK VERIFY PASSED!\r\n"
        );
    }
    else
    {
        Cy_SCB_UART_PutString(
            SCB3,
            "[FATFS Error] READBACK DATA MISMATCH!\r\n"
        );

        return;
    }


    /* =====================================================
     * TEST COMPLETE
     * ===================================================== */

    Cy_SCB_UART_PutString(
        SCB3,
        "\r\n"
        "SD CARD FATFS TEST COMPLETE.\r\n"
    );


    /* =====================================================
     * Application successfully initialized
     * ===================================================== */

    sd_card_initialized = true;


    Cy_SCB_UART_PutString(
        SCB3,
        "PSOC: OK: SD CARD\r\n"
    );
}


/* =========================================================
 * SD CARD APPLICATION RUN
 * ========================================================= */

void sd_card_app_run(void)
{
    /*
     * The SD test is intentionally performed only once
     * during sd_card_app_init().
     *
     * Do not continuously write to the SD card here.
     */

    if (!sd_card_initialized)
    {
        return;
    }
}


/* =========================================================
 * SD CARD APPLICATION STOP
 * ========================================================= */

void sd_card_app_stop(void)
{
    sd_card_initialized = false;

    /*
     * Do not perform another FATFS operation here.
     */
}
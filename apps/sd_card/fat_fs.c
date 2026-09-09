#include "fat_fs.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static FATFS fatfs;
static bool mounted = false;


bool fat_fs_mount(void)
{
    FRESULT res;

    mounted = false;

    res = f_mount(
        &fatfs,
        "",
        1
    );

    if (res == FR_OK)
    {
        mounted = true;
        return true;
    }

    return false;
}


bool fat_fs_mkdir(const char *folder_name)
{
    if (!mounted)
    {
        if (!fat_fs_mount())
            return false;
    }

    FRESULT res = f_mkdir(folder_name);

    if (res == FR_OK || res == FR_EXIST)
        return true;

    return false;
}


bool fat_fs_write_file(
    const char *folder_name,
    const char *filename,
    const char *text)
{
    if (!mounted)
    {
        if (!fat_fs_mount())
            return false;
    }


    /*
     * Create directory if necessary.
     */
    if (!fat_fs_mkdir(folder_name))
    {
        /*
         * It may already exist.
         * Continue only if it actually exists.
         */
    }


    char path[64];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        folder_name,
        filename
    );


    FIL file;

    FRESULT res = f_open(
        &file,
        path,
        FA_CREATE_ALWAYS | FA_WRITE
    );

    if (res != FR_OK)
        return false;


    const UINT length = (UINT)strlen(text);

    UINT written = 0;

    res = f_write(
        &file,
        text,
        length,
        &written
    );


    /*
     * Make sure data reaches SD card.
     */
    if (res == FR_OK)
        res = f_sync(&file);


    FRESULT close_res = f_close(&file);

    if (res != FR_OK)
        return false;

    if (close_res != FR_OK)
        return false;

    return (written == length);
}


bool fat_fs_read_file(
    const char *folder_name,
    const char *filename,
    char *buffer,
    size_t buffer_size)
{
    if (!mounted)
    {
        if (!fat_fs_mount())
            return false;
    }


    if (buffer == NULL || buffer_size < 2U)
        return false;


    char path[64];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        folder_name,
        filename
    );


    FIL file;

    FRESULT res = f_open(
        &file,
        path,
        FA_READ
    );

    if (res != FR_OK)
        return false;


    UINT bytes_read = 0;

    res = f_read(
        &file,
        buffer,
        (UINT)(buffer_size - 1U),
        &bytes_read
    );


    f_close(&file);


    if (res != FR_OK)
        return false;


    buffer[bytes_read] = '\0';

    return true;
}
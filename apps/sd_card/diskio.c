#include "ff.h"
#include "diskio.h"
#include "sd_card.h"

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
        return STA_NOINIT;

    if (sd_init())
        return 0;

    return STA_NOINIT;
}


DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0)
        return STA_NOINIT;

    if (sd_is_ready())
        return 0;

    return STA_NOINIT;
}


DRESULT disk_read(
    BYTE pdrv,
    BYTE *buff,
    LBA_t sector,
    UINT count)
{
    if (pdrv != 0 || buff == NULL || count == 0)
        return RES_PARERR;

    if (!sd_is_ready())
        return RES_NOTRDY;


    for (UINT i = 0; i < count; i++)
    {
        if (!sd_read_sector(
                (uint32_t)(sector + i),
                buff + (i * 512U)))
        {
            return RES_ERROR;
        }
    }

    return RES_OK;
}


#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,
    const BYTE *buff,
    LBA_t sector,
    UINT count)
{
    if (pdrv != 0 || buff == NULL || count == 0)
        return RES_PARERR;

    if (!sd_is_ready())
        return RES_NOTRDY;


    for (UINT i = 0; i < count; i++)
    {
        if (!sd_write_sector(
                (uint32_t)(sector + i),
                buff + (i * 512U)))
        {
            return RES_ERROR;
        }
    }

    return RES_OK;
}

#endif


DRESULT disk_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff)
{
    if (pdrv != 0)
        return RES_PARERR;

    if (!sd_is_ready())
        return RES_NOTRDY;


    switch (cmd)
    {
        case CTRL_SYNC:
            return RES_OK;


        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512U;
            return RES_OK;


        case GET_BLOCK_SIZE:
            /*
             * Minimum erase block size.
             * 1 is acceptable when the exact value
             * isn't required.
             */
            *(DWORD *)buff = 1UL;
            return RES_OK;


        case GET_SECTOR_COUNT:
            /*
             * FatFs mainly needs this for formatting and
             * free-space calculations.
             *
             * We can obtain the real value from CSD,
             * but don't need it for the basic read/write
             * test.
             */
            *(LBA_t *)buff = 7744512UL;
            return RES_OK;


        default:
            return RES_PARERR;
    }
}
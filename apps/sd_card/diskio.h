#ifndef DISKIO_H
#define DISKIO_H

#include "ff.h"


/* Disk Status Bits */

#define STA_NOINIT      0x01
#define STA_NODISK      0x02
#define STA_PROTECT     0x04


/* Results of Disk Functions */

typedef BYTE DSTATUS;

typedef enum
{
    RES_OK = 0,
    RES_ERROR,
    RES_WRPRT,
    RES_NOTRDY,
    RES_PARERR
} DRESULT;


/* Generic command */

#define CTRL_SYNC           0
#define GET_SECTOR_COUNT    1
#define GET_SECTOR_SIZE     2
#define GET_BLOCK_SIZE      3


/* Disk Functions */

DSTATUS disk_initialize(BYTE pdrv);

DSTATUS disk_status(BYTE pdrv);

DRESULT disk_read(
    BYTE pdrv,
    BYTE *buff,
    LBA_t sector,
    UINT count
);

#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,
    const BYTE *buff,
    LBA_t sector,
    UINT count
);

#endif

DRESULT disk_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff
);

#endif /* DISKIO_H */
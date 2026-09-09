#ifndef FAT_FS_H
#define FAT_FS_H

#include <stdbool.h>
#include <stddef.h>

bool fat_fs_mount(void);

bool fat_fs_mkdir(
    const char *folder_name
);

bool fat_fs_write_file(
    const char *folder_name,
    const char *filename,
    const char *text
);

bool fat_fs_read_file(
    const char *folder_name,
    const char *filename,
    char *buffer,
    size_t buffer_size
);

#endif
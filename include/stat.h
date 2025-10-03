#pragma once
#include <stdint.h>
#include <stddef.h>

struct stat {
    uint64_t st_dev;     // unused (0)
    uint64_t st_ino;     // use pointer value hash
    uint32_t st_mode;    // file type + perms (simple)
    uint32_t st_nlink;   // always 1
    uint32_t st_uid;     // 0
    uint32_t st_gid;     // 0
    uint64_t st_rdev;    // 0
    uint64_t st_size;    // file size
    uint64_t st_blksize; // 512
    uint64_t st_blocks;  // size/512 rounded
    uint64_t st_atime;   // 0 (placeholder)
    uint64_t st_mtime;   // 0
    uint64_t st_ctime;   // 0
};

#define S_IFDIR  0040000
#define S_IFREG  0100000
#define S_IRUSR  0000400
#define S_IWUSR  0000200
#define S_IRGRP  0000040
#define S_IROTH  0000004


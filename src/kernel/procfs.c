/* procfs.c - Simple /proc filesystem implementation */
#include "../fs/fs.h"
#include "kprint.h"
#include "proc.h"
#include "pmm.h"
#include <stdint.h>

/* Helper to create pseudo-files in /proc */
static char meminfo_buf[512];
static char cpuinfo_buf[512];
static char version_buf[256];

static void update_meminfo(void)
{
    /* Simple memory info - adjust based on actual PMM */
    const char *info = "MemTotal:     1048576 kB\n"
                       "MemFree:       524288 kB\n"
                       "MemAvailable:  524288 kB\n"
                       "Buffers:            0 kB\n"
                       "Cached:             0 kB\n"
                       "SwapTotal:          0 kB\n"
                       "SwapFree:           0 kB\n";
    int i = 0;
    while (info[i] && i < 511) {
        meminfo_buf[i] = info[i];
        i++;
    }
    meminfo_buf[i] = '\0';
}

static void update_cpuinfo(void)
{
    const char *info = "processor  : 0\n"
                       "vendor_id  : GenuineIntel\n"
                       "cpu family : 6\n"
                       "model      : 0\n"
                       "model name : SnowKernel Virtual CPU\n"
                       "stepping   : 0\n"
                       "cpu MHz    : 2400.000\n"
                       "cache size : 256 KB\n"
                       "flags      : fpu vme de pse tsc msr pae\n";
    int i = 0;
    while (info[i] && i < 511) {
        cpuinfo_buf[i] = info[i];
        i++;
    }
    cpuinfo_buf[i] = '\0';
}

static void update_version(void)
{
    const char *info = "SnowKernel version 0.1.0 (x86_64)\n";
    int i = 0;
    while (info[i] && i < 255) {
        version_buf[i] = info[i];
        i++;
    }
    version_buf[i] = '\0';
}

void procfs_init(void)
{
    node_t *root = fs_root();
    if (!root) return;
    
    kprintf("[procfs] creating /proc filesystem...\n");
    
    /* Create /proc directory */
    node_t *proc = fs_mkdir(root, "proc");
    if (!proc) {
        kprintf("[procfs] failed to create /proc\n");
        return;
    }
    
    /* Update pseudo-file contents */
    update_meminfo();
    update_cpuinfo();
    update_version();
    
    /* Create /proc/meminfo */
    node_t *meminfo = fs_create_file(proc, "meminfo");
    if (meminfo) {
        int len = 0;
        while (meminfo_buf[len]) len++;
        fs_write(meminfo, meminfo_buf, len, 0);
        kprintf("[procfs] created /proc/meminfo\n");
    }
    
    /* Create /proc/cpuinfo */
    node_t *cpuinfo = fs_create_file(proc, "cpuinfo");
    if (cpuinfo) {
        int len = 0;
        while (cpuinfo_buf[len]) len++;
        fs_write(cpuinfo, cpuinfo_buf, len, 0);
        kprintf("[procfs] created /proc/cpuinfo\n");
    }
    
    /* Create /proc/version */
    node_t *version = fs_create_file(proc, "version");
    if (version) {
        int len = 0;
        while (version_buf[len]) len++;
        fs_write(version, version_buf, len, 0);
        kprintf("[procfs] created /proc/version\n");
    }
    
    /* Create /proc/self directory */
    node_t *self = fs_mkdir(proc, "self");
    if (self) {
        /* Create /proc/self/cmdline */
        node_t *cmdline = fs_create_file(self, "cmdline");
        if (cmdline) {
            const char *cmd = "init\0";
            fs_write(cmdline, cmd, 5, 0);
        }
        
        /* Create /proc/self/status */
        node_t *status = fs_create_file(self, "status");
        if (status) {
            const char *st = "Name:\tinit\n"
                            "State:\tR (running)\n"
                            "Pid:\t1\n"
                            "PPid:\t0\n"
                            "Uid:\t0\t0\t0\t0\n"
                            "Gid:\t0\t0\t0\t0\n";
            int len = 0;
            while (st[len]) len++;
            fs_write(status, st, len, 0);
        }
        
        kprintf("[procfs] created /proc/self\n");
    }
    
    /* Create /proc/1 directory (for PID 1) */
    node_t *pid1 = fs_mkdir(proc, "1");
    if (pid1) {
        node_t *pid1_cmdline = fs_create_file(pid1, "cmdline");
        if (pid1_cmdline) {
            const char *cmd = "init\0";
            fs_write(pid1_cmdline, cmd, 5, 0);
        }
        
        node_t *pid1_status = fs_create_file(pid1, "status");
        if (pid1_status) {
            const char *st = "Name:\tinit\n"
                            "State:\tR (running)\n"
                            "Pid:\t1\n"
                            "PPid:\t0\n"
                            "Uid:\t0\t0\t0\t0\n"
                            "Gid:\t0\t0\t0\t0\n";
            int len = 0;
            while (st[len]) len++;
            fs_write(pid1_status, st, len, 0);
        }
        
        kprintf("[procfs] created /proc/1\n");
    }
    
    kprintf("[procfs] initialization complete\n");
}

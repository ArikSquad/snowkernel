/* procfs.c - /proc filesystem implementation */
#include "../fs/fs.h"
#include "kprint.h"
#include "proc.h"
#include "pmm.h"
#include <stdint.h>

extern uintptr_t pmm_cur;
extern uintptr_t pmm_end;
extern void *pmm_free_list;
extern volatile uint64_t ticks;

/* Helper to create pseudo-files in /proc */
static char meminfo_buf[512];
static char cpuinfo_buf[512];
static char version_buf[256];

static void update_meminfo(void)
{
    /* Get real memory info from PMM */
    extern uintptr_t pmm_cur;
    extern uintptr_t pmm_end;
    
    size_t total_kb = (pmm_end / 1024);
    size_t used_kb = (pmm_cur / 1024);
    size_t free_kb = total_kb - used_kb;
    
    /* Count free list pages */
    extern void *pmm_free_list;
    int free_pages = 0;
    void *p = pmm_free_list;
    while (p && free_pages < 10000) {  /* Safety limit */
        free_pages++;
        p = *(void**)p;
    }
    free_kb += (free_pages * 4);  /* 4KB pages */
    
    char buf[512];
    int pos = 0;
    
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "MemTotal:     %u kB\n", (unsigned)total_kb);
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "MemFree:      %u kB\n", (unsigned)free_kb);
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "MemAvailable: %u kB\n", (unsigned)free_kb);
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "Buffers:            0 kB\n");
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "Cached:             0 kB\n");
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "SwapTotal:          0 kB\n");
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "SwapFree:           0 kB\n");
    
    for (int i = 0; i < pos && i < 511; i++) {
        meminfo_buf[i] = buf[i];
    }
    meminfo_buf[pos < 511 ? pos : 511] = '\0';
}

static void read_cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(0));
}

static void update_cpuinfo(void)
{
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];
    
    /* Get vendor string */
    read_cpuid(0, &eax, &ebx, &ecx, &edx);
    *(uint32_t*)(vendor + 0) = ebx;
    *(uint32_t*)(vendor + 4) = edx;
    *(uint32_t*)(vendor + 8) = ecx;
    vendor[12] = '\0';
    
    /* Get features */
    read_cpuid(1, &eax, &ebx, &ecx, &edx);
    uint32_t family = ((eax >> 8) & 0xF) + ((eax >> 20) & 0xFF);
    uint32_t model = ((eax >> 4) & 0xF) | ((eax >> 12) & 0xF0);
    uint32_t stepping = eax & 0xF;
    
    char buf[512];
    int pos = 0;
    
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "processor  : 0\n");
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "vendor_id  : %s\n", vendor);
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "cpu family : %u\n", (unsigned)family);
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "model      : %u\n", (unsigned)model);
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "model name : x86_64 Processor\n");
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "stepping   : %u\n", (unsigned)stepping);
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "flags      :");
    if (edx & (1 << 0)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " fpu");
    if (edx & (1 << 1)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " vme");
    if (edx & (1 << 2)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " de");
    if (edx & (1 << 3)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " pse");
    if (edx & (1 << 4)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " tsc");
    if (edx & (1 << 5)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " msr");
    if (edx & (1 << 6)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " pae");
    if (edx & (1 << 9)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " apic");
    if (edx & (1 << 15)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " cmov");
    if (edx & (1 << 23)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " mmx");
    if (edx & (1 << 25)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " sse");
    if (edx & (1 << 26)) pos += ksnprintf(buf + pos, sizeof(buf) - pos, " sse2");
    pos += ksnprintf(buf + pos, sizeof(buf) - pos, "\n");
    
    for (int i = 0; i < pos && i < 511; i++) {
        cpuinfo_buf[i] = buf[i];
    }
    cpuinfo_buf[pos < 511 ? pos : 511] = '\0';
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

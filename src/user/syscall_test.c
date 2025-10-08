/* syscall_test.c - Test program for new syscalls */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("=== SnowKernel Syscall Test Program ===\n\n");
    
    /* Test process identification syscalls */
    printf("Testing process identification syscalls:\n");
    int pid = getpid();
    int ppid = getppid();
    printf("  getpid() = %d\n", pid);
    printf("  getppid() = %d\n", ppid);
    
    /* Test user/group ID syscalls */
    printf("\nTesting user/group ID syscalls:\n");
    int uid = getuid();
    int gid = getgid();
    int euid = geteuid();
    int egid = getegid();
    printf("  getuid() = %d\n", uid);
    printf("  getgid() = %d\n", gid);
    printf("  geteuid() = %d\n", euid);
    printf("  getegid() = %d\n", egid);
    
    /* Test directory operations */
    printf("\nTesting directory operations:\n");
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("  getcwd() = '%s'\n", cwd);
    } else {
        printf("  getcwd() failed\n");
    }
    
    /* Test mkdir */
    printf("  mkdir('/tmp/test') = ");
    if (mkdir("/tmp/test", 0755) == 0) {
        printf("success\n");
    } else {
        printf("failed (errno=%d)\n", errno);
    }
    
    /* Test chdir */
    printf("  chdir('/tmp') = ");
    if (chdir("/tmp") == 0) {
        printf("success\n");
        if (getcwd(cwd, sizeof(cwd))) {
            printf("    new cwd = '%s'\n", cwd);
        }
    } else {
        printf("failed\n");
    }
    
    /* Test file operations */
    printf("\nTesting file operations:\n");
    
    /* Test access */
    printf("  access('/proc/meminfo', 0) = ");
    if (access("/proc/meminfo", 0) == 0) {
        printf("success (file exists)\n");
    } else {
        printf("failed\n");
    }
    
    /* Test reading /proc/meminfo */
    printf("\nReading /proc/meminfo:\n");
    int fd = open("/proc/meminfo", 0, 0);
    if (fd >= 0) {
        char buf[256];
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s", buf);
        }
        close(fd);
    } else {
        printf("  Failed to open /proc/meminfo\n");
    }
    
    /* Test reading /proc/cpuinfo */
    printf("\nReading /proc/cpuinfo:\n");
    fd = open("/proc/cpuinfo", 0, 0);
    if (fd >= 0) {
        char buf[256];
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s", buf);
        }
        close(fd);
    } else {
        printf("  Failed to open /proc/cpuinfo\n");
    }
    
    /* Test umask */
    printf("\nTesting umask:\n");
    int old_mask = umask(0022);
    printf("  umask(0022) returned %04o\n", old_mask);
    
    /* Test time syscalls */
    printf("\nTesting time syscalls:\n");
    long t = time(NULL);
    printf("  time(NULL) = %ld\n", t);
    
    struct timeval {
        long tv_sec;
        long tv_usec;
    } tv;
    if (gettimeofday(&tv, NULL) == 0) {
        printf("  gettimeofday() = %ld.%06ld\n", tv.tv_sec, tv.tv_usec);
    }
    
    printf("\n=== All tests complete ===\n");
    return 0;
}

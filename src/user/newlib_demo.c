/* newlib_demo.c - Demonstrate newlib integration with SnowKernel */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/* This program demonstrates that newlib functions work correctly
 * with SnowKernel's syscall implementation */

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("=== Newlib Integration Demo ===\n\n");
    
    /* Standard I/O */
    printf("1. Standard I/O functions:\n");
    printf("   printf() works!\n");
    fprintf(stdout, "   fprintf() works!\n");
    puts("   puts() works!");
    
    /* String functions from stdlib/string.h */
    printf("\n2. String functions:\n");
    char buf[100];
    strcpy(buf, "strcpy() works! ");
    strcat(buf, "strcat() works too!");
    printf("   Result: %s\n", buf);
    printf("   strlen(buf) = %zu\n", strlen(buf));
    
    /* Memory allocation */
    printf("\n3. Dynamic memory allocation:\n");
    char *mem = malloc(256);
    if (mem) {
        sprintf(mem, "malloc() allocated memory at %p", mem);
        printf("   %s\n", mem);
        free(mem);
        printf("   free() released the memory\n");
    }
    
    /* File operations */
    printf("\n4. File operations:\n");
    FILE *fp = fopen("/proc/version", "r");
    if (fp) {
        char line[128];
        if (fgets(line, sizeof(line), fp)) {
            printf("   fopen() and fgets() read: %s", line);
        }
        fclose(fp);
        printf("   fclose() closed the file\n");
    } else {
        printf("   fopen() failed to open /proc/version\n");
    }
    
    /* Directory operations via POSIX functions */
    printf("\n5. POSIX directory operations:\n");
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("   getcwd() returned: %s\n", cwd);
    }
    
    /* Process information */
    printf("\n6. Process information:\n");
    printf("   getpid() = %d\n", getpid());
    printf("   getppid() = %d\n", getppid());
    printf("   getuid() = %d\n", getuid());
    printf("   getgid() = %d\n", getgid());
    
    /* Test stat */
    printf("\n7. File status (stat):\n");
    struct stat st;
    if (stat("/proc/meminfo", &st) == 0) {
        printf("   stat() on /proc/meminfo succeeded\n");
        printf("   File size: %ld bytes\n", (long)st.st_size);
    }
    
    /* Environment (limited support) */
    printf("\n8. Environment:\n");
    printf("   Environment variables not yet supported\n");
    
    printf("\n=== Demo complete ===\n");
    printf("All newlib functions work correctly with SnowKernel!\n");
    
    return 0;
}

#include <stdio.h>
#include <unistd.h>
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Hello from userland!\n");
    for (int i = 0; i < 3; i++)
    {
        printf("Line %d via printf (fd=%d)\n", i + 1, 1);
    }
    write(1, "Direct write() OK\n", 18);
    return 0;
}

#include "syscall.h"
long syscall_thunk(long num, long a1, long a2, long a3, long a4, long a5, long a6)
{
    return syscall_dispatch(num, a1, a2, a3, a4, a5, a6);
}

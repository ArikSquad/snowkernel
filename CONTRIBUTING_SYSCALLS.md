# Adding New System Calls to SnowKernel

This guide explains how to add new system calls to SnowKernel.

## Overview

System calls in SnowKernel follow this flow:
1. User program invokes syscall via `int 0x80`
2. Interrupt handler calls `syscall_thunk` (in `src/cpu/int80.c`)
3. `syscall_thunk` calls `syscall_dispatch` (in `src/kernel/syscall.c`)
4. `syscall_dispatch` routes to appropriate `sys_*` function
5. Result is returned to user program

## Step-by-Step Guide

### 1. Add Syscall Number

Edit `include/syscall.h` and add your syscall to the enum:

```c
enum {
    // ... existing syscalls ...
    SYS_your_syscall = 123,  // Choose unused number
};
```

**Note**: Try to match Linux syscall numbers when possible for compatibility.

### 2. Add Function Declaration

In `include/syscall.h`, add the function prototype:

```c
long sys_your_syscall(int arg1, void *arg2);
```

**Guidelines**:
- Return type should be `long` (negative for errors, non-negative for success)
- Use appropriate parameter types
- Follow naming convention: `sys_*`

### 3. Implement the Syscall

In `src/kernel/syscall.c`, implement the function:

```c
long sys_your_syscall(int arg1, void *arg2)
{
    // Validate parameters
    if (!arg2) return -1;
    
    // Implement the syscall logic
    // ...
    
    // Return result (negative = error, non-negative = success)
    return 0;
}
```

**Best Practices**:
- Always validate pointers and parameters
- Return -1 (or negative errno) on error
- Use existing helper functions (proc_current(), fs_lookup(), etc.)
- Add kernel logging with kprintf() for debugging
- Document edge cases and limitations

### 4. Add to Dispatcher

In `src/kernel/syscall.c`, add a case to `syscall_dispatch()`:

```c
long syscall_dispatch(long num, long a1, long a2, long a3, ...)
{
    switch (num) {
        // ... existing cases ...
        case SYS_your_syscall:
            return sys_your_syscall((int)a1, (void *)a2);
        // ...
    }
}
```

**Casting Guidelines**:
- `a1-a6` are `long` values passed from user space
- Cast to appropriate types based on your function signature
- Be careful with pointer casts - validate them in your implementation

### 5. Add Userspace Wrapper

In `src/libc/syscalls.c`, add a wrapper function:

```c
int _your_syscall(int arg1, void *arg2)
{
    long r = ksys(SYS_your_syscall, arg1, (long)arg2, 0, 0, 0, 0);
    return (int)r;
}
```

Then add weak symbol aliases:

```c
#ifdef __GNUC__
int your_syscall(int arg1, void *arg2) __attribute__((weak, alias("_your_syscall")));
#else
int your_syscall(int arg1, void *arg2) { return _your_syscall(arg1, arg2); }
#endif
```

### 6. Test the Syscall

Create or update a test program in `src/user/`:

```c
#include <unistd.h>
#include <stdio.h>

// Declare if not in headers
extern int your_syscall(int arg1, void *arg2);

int main() {
    int result = your_syscall(42, NULL);
    printf("your_syscall returned: %d\n", result);
    return 0;
}
```

### 7. Document the Syscall

Add documentation to `SYSCALLS.md`:

```markdown
#### `SYS_your_syscall (123)`
```c
long sys_your_syscall(int arg1, void *arg2);
```
Description of what the syscall does, its parameters, return value, and any limitations.
```

## Example: Adding `getpid`

Here's a complete example of adding the `getpid` syscall:

**1. include/syscall.h:**
```c
enum {
    // ...
    SYS_getpid = 39,
};

long sys_getpid(void);
```

**2. src/kernel/syscall.c:**
```c
long sys_getpid(void)
{
    process_t *p = proc_current();
    return p ? p->pid : 1;
}

long syscall_dispatch(long num, long a1, ...) {
    switch (num) {
        // ...
        case SYS_getpid:
            return sys_getpid();
    }
}
```

**3. src/libc/syscalls.c:**
```c
int _getpid(void) { 
    long r = ksys(SYS_getpid, 0, 0, 0, 0, 0, 0);
    return (int)r;
}

#ifdef __GNUC__
int getpid(void) __attribute__((weak, alias("_getpid")));
#endif
```

**4. Test:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    printf("My PID: %d\n", getpid());
    return 0;
}
```

## Common Patterns

### File Descriptor Operations
```c
long sys_your_fd_op(int fd, ...) {
    fd_entry_t *e = proc_get_fd(fd);
    if (!e) return -1;
    
    node_t *n = e->node;
    if (!n) return -1;
    
    // Operate on the file
}
```

### Path Operations
```c
long sys_your_path_op(const char *path, ...) {
    if (!path) return -1;
    
    node_t *cwd = fs_cwd();
    node_t *n = fs_lookup(cwd, path);
    if (!n) return -1;
    
    // Operate on the file/directory
}
```

### Process Operations
```c
long sys_your_proc_op(...) {
    process_t *p = proc_current();
    if (!p) return -1;
    
    // Operate on the process
}
```

## Error Handling

Return these error codes:
- `-1`: Generic error (most common)
- `-EINVAL`: Invalid argument (if errno.h is available)
- `-ENOENT`: No such file or directory
- `-EACCES`: Permission denied
- etc.

For now, most syscalls just return `-1` on error since errno support is minimal.

## Testing Checklist

- [ ] Syscall compiles without warnings
- [ ] Syscall number doesn't conflict with existing ones
- [ ] Syscall is in dispatcher switch statement
- [ ] Userspace wrapper is implemented
- [ ] Test program exercises the syscall
- [ ] Error cases are handled gracefully
- [ ] Documentation is added to SYSCALLS.md
- [ ] Code follows kernel style (2-space indent, etc.)

## Tips

1. **Start Simple**: Implement a basic version first, then add features
2. **Check Existing Code**: Look at similar syscalls for patterns
3. **Validate Everything**: Never trust user pointers or parameters
4. **Log for Debug**: Use `kprintf()` to trace syscall execution
5. **Test Edge Cases**: NULL pointers, invalid FDs, empty strings, etc.
6. **Keep it Minimal**: Don't over-engineer - simple implementations are better

## References

- Linux syscall table: https://filippo.io/linux-syscall-table/
- POSIX specification: https://pubs.opengroup.org/onlinepubs/9699919799/
- Existing syscalls: `src/kernel/syscall.c`

#pragma once
// Unified minimal stdarg shim. We may shadow GCC/newlib's stdarg.h because
// our include path (-Iinclude) precedes theirs for some builds. Provide the
// intrinsic-based implementation and also define the identifier newlib's
// headers expect: __gnuc_va_list.

#ifndef __gnuc_va_list
typedef __builtin_va_list __gnuc_va_list;
#endif
typedef __gnuc_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

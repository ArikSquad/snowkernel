#pragma once
#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __DEFINED_size_t
#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 8
typedef unsigned long size_t;
#else
typedef unsigned int size_t;
#endif
#define __DEFINED_size_t 1
#endif

typedef int ptrdiff_t;

typedef unsigned int wchar_t; /* placeholder */

#ifndef _WINT_T
#define _WINT_T
typedef unsigned int wint_t; /* match newlib default underlying type */
#endif

#pragma once
#include <stddef.h>
#include <stdint.h>

size_t kstrlen(const char* s);
int kstrcmp(const char* a, const char* b);
int kstrncmp(const char* a, const char* b, size_t n);
char* kstrcpy(char* dst, const char* src);
char* kstrncpy(char* dst, const char* src, size_t n);
char* kstrcat(char* dst, const char* src);
char* kstrchr(const char* s, int c);
void* kmemset(void* dst, int v, size_t n);
void* kmemcpy(void* dst, const void* src, size_t n);
char* kstrdup(const char* s);

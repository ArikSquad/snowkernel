#include "string.h"
#include <stdint.h>

size_t kstrlen(const char* s){ size_t n=0; while(s&&*s++){n++;} return n; }
int kstrcmp(const char* a,const char* b){ while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b; }
int kstrncmp(const char* a,const char* b,size_t n){ for(size_t i=0;i<n;i++){ if(a[i]!=b[i]||!a[i]||!b[i]) return (unsigned char)a[i]-(unsigned char)b[i]; } return 0; }
char* kstrcpy(char* d,const char* s){ char* r=d; while((*d++=*s++)); return r; }
char* kstrncpy(char* d,const char* s,size_t n){ size_t i=0; for(;i<n&&s[i];++i) d[i]=s[i]; for(;i<n;++i) d[i]='\0'; return d; }
char* kstrcat(char* d,const char* s){ char* r=d; while(*d)d++; while((*d++=*s++)); return r; }
char* kstrchr(const char* s,int c){ while(*s){ if(*s==c) return (char*)s; s++; } return c==0?(char*)s:0; }
void* kmemset(void* d,int v,size_t n){ unsigned char* p=d; for(size_t i=0;i<n;++i) p[i]=(unsigned char)v; return d; }
void* kmemcpy(void* d,const void* s,size_t n){ unsigned char* dd=d; const unsigned char* ss=s; for(size_t i=0;i<n;++i) dd[i]=ss[i]; return d; }
char* kstrdup(const char* s){ size_t n=kstrlen(s); char* d=(char*)0; (void)n; return d; }

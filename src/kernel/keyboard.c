#include "keyboard.h"
#include "kprint.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port){ uint8_t r; __asm__ __volatile__("inb %1, %0" : "=a"(r) : "Nd"(port)); return r; }

static const char keymap[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,9,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
    'd','f','g','h','j','k','l',';','\'', '`',0,'\\','z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,' ',0
};

int kbd_read_line(char* buf, int max){
    int i=0; for(;;){
        while(!(inb(0x64) & 1)){}
        uint8_t sc=inb(0x60);
        if(sc&0x80) continue;
        char c = (sc<128)?keymap[sc]:0;
        if(c==0) continue;
        if(c=='\n') { kputc('\n'); buf[i]=0; return i; }
        if(c==8){ if(i>0){ i--; kputc('\b'); kputc(' '); kputc('\b'); } continue; }
        if(i<max-1){ buf[i++]=c; kputc(c); }
    }
}

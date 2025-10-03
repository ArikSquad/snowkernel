#include "kprint.h"
#include "../fs/fs.h"
#include "shell.h"

void kernel_main(void){
    kset_color(7,0);
    kclear();
    kputs("YapKernel\n");
    fs_init();
    fs_mkdir(fs_root(),"home");
    shell_run();
}

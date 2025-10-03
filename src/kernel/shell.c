#include "shell.h"
#include "kprint.h"
#include "keyboard.h"
#include "../fs/fs.h"
#include "string.h"

typedef void (*cmd_fn)(char*);
typedef struct { const char* name; const char* desc; cmd_fn fn; } cmd_t;

static char redir_buf[4096];
static unsigned redir_len=0;
static void redir_sink_char(char c){ if(redir_len < sizeof(redir_buf)) redir_buf[redir_len++] = c; }

static void get_cwd_path(char* out, int max){
    node_t* stack[32]; int sp=0; node_t* cur=fs_cwd();
    while(cur){ stack[sp++]=cur; cur=cur->parent; if(sp>=32) break; }
    int j=0; if(sp==1){ out[0]='/'; out[1]='\0'; return; }
    for(int i=sp-1;i>=0;i--){ if(i==sp-1) continue; out[j++]='/'; char* n=stack[i]->name; for(int k=0;n[k] && j<max-1;k++) out[j++]=n[k]; if(j>=max-1) break; }
    out[j]='\0';
}

static void prompt(){ char p[128]; get_cwd_path(p,128); kprintf("%s $ ", p); }

static void builtin_help(char*);
static void builtin_clear(char*);
static void builtin_echo(char*);
static void builtin_cat(char*);
static void builtin_mkdir(char*);
static void builtin_cd(char*);

static cmd_t CMDS[] = {
    {"help",  "List commands", builtin_help},
    {"clear", "Clear screen", builtin_clear},
    {"echo",  "Print text", builtin_echo},
    {"cat",   "Show file contents", builtin_cat},
    {"mkdir", "Create directory", builtin_mkdir},
    {"cd",    "Change directory", builtin_cd},
};
static const int CMDS_N = sizeof(CMDS)/sizeof(CMDS[0]);

static void builtin_help(char* args){ (void)args; for(int i=0;i<CMDS_N;i++){ kprintf("%s - %s\n", CMDS[i].name, CMDS[i].desc); } }
static void builtin_clear(char* args){ (void)args; kclear(); }

static int parse_redir(char* line, char** left, char** file, int* append){
    *append=0; *file=0; *left=line;
    char* p = kstrchr(line,'>');
    if(!p) return 0;
    *p='\0'; p++;
    if(*p=='>'){ *append=1; p++; }
    while(*p==' ') p++;
    if(*p=='\0') return 0;
    *file=p; return 1;
}

static void cmd_write_to(node_t* f, const char* s, int append){ fs_write(f,s,kstrlen(s),append); fs_write(f,"\n",1,1); }
static void builtin_echo(char* args){ kputs(args); kputc('\n'); }

static void builtin_cat(char* args){
    node_t* n = fs_lookup(fs_cwd(), args);
    if(!n || n->type!=NODE_FILE){ kputs("not a file\n"); return; }
    char buf[512]; int r=fs_read(n,buf,sizeof(buf)-1); buf[r]=0; kputs(buf); kputc('\n');
}

static void builtin_mkdir(char* args){ if(!args||!*args){ kputs("usage: mkdir <name>\n"); return; } if(!fs_mkdir(fs_cwd(),args)) kputs("error\n"); }

static void builtin_cd(char* args){ node_t* n=fs_lookup(fs_cwd(),args); if(n && n->type==NODE_DIR) fs_set_cwd(n); else kputs("no such dir\n"); }

void shell_run(void){
    char line[256];
    for(;;){
        prompt();
        int n = kbd_read_line(line, sizeof(line));
        if(n<=0) continue;
        char* file=0; int append=0; parse_redir(line,&line,&file,&append);
    char* cmd=line; while(*cmd==' ') cmd++;
    char* args=cmd; while(*args && *args!=' ') args++; if(*args){ *args='\0'; args++; while(*args==' ') args++; }
        int handled=0;
        for(int i=0;i<CMDS_N;i++) if(kstrcmp(cmd,CMDS[i].name)==0){
            if(file){ node_t* f=fs_create_file(fs_cwd(),file); if(!f){ kputs("redir error\n"); handled=1; break; }
                redir_len = 0; kputc_fn prev = kprint_set_sink(redir_sink_char);
                CMDS[i].fn(args?args:(char*)"");
                kprint_set_sink(prev);
                fs_write(f, redir_buf, redir_len, append);
            } else {
                CMDS[i].fn(args?args:(char*)"");
            }
            handled=1; break; }
        if(!handled) kputs("unknown\n");
    }
}

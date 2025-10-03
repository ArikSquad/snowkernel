#include "fs.h"
#include "../kernel/string.h"
#include <stdint.h>

static node_t root;
static node_t* cwd;
static node_t nodes[512];
static int ni=0;
static char arena[131072];
static size_t aoff=0;

static node_t* add_child(node_t* p, node_t* n){
    n->parent=p; n->sibling=0; n->child=0; n->data=0; n->size=0;
    if(!p->child) p->child=n; else { node_t* c=p->child; while(c->sibling) c=c->sibling; c->sibling=n; }
    return n;
}

void fs_init(void){
    kmemset(&root,0,sizeof(root));
    kstrcpy(root.name, "/");
    root.type=NODE_DIR; root.parent=0; root.child=0; root.sibling=0;
    cwd=&root;
}

node_t* fs_root(void){ return &root; }
node_t* fs_cwd(void){ return cwd; }
void fs_set_cwd(node_t* n){ if(n&&n->type==NODE_DIR) cwd=n; }

static node_t* find_in(node_t* p, const char* name){ for(node_t* c=p->child;c;c=c->sibling) if(kstrcmp(c->name,name)==0) return c; return 0; }

node_t* fs_mkdir(node_t* parent, const char* name){ if(!parent||parent->type!=NODE_DIR) return 0; if(find_in(parent,name)) return 0; if(ni>=512) return 0; node_t* n=&nodes[ni++]; kmemset(n,0,sizeof(*n)); kstrncpy(n->name,name,31); n->type=NODE_DIR; return add_child(parent,n); }

node_t* fs_create_file(node_t* parent, const char* name){ if(!parent||parent->type!=NODE_DIR) return 0; node_t* e=find_in(parent,name); if(e){ if(e->type==NODE_FILE) return e; else return 0; } if(ni>=512) return 0; node_t* n=&nodes[ni++]; kmemset(n,0,sizeof(*n)); kstrncpy(n->name,name,31); n->type=NODE_FILE; return add_child(parent,n); }

node_t* fs_lookup(node_t* parent, const char* path){ if(!path||!*path) return parent; if(path[0]=='/') parent=&root; char tmp[128]; kstrncpy(tmp,path,127); char* s=tmp; node_t* cur=parent; char* tok=s; for(size_t i=0;;++i){ if(s[i]=='/'||s[i]=='\0'){ char c=s[i]; s[i]='\0'; if(kstrcmp(tok,"")!=0){ if(kstrcmp(tok,".")==0){ } else if(kstrcmp(tok,"..")==0){ if(cur->parent) cur=cur->parent; } else { node_t* f=find_in(cur,tok); if(!f) return 0; cur=f; } } if(c=='\0') break; tok=&s[i+1]; }
    return cur; }

int fs_write(node_t* f, const char* data, size_t len, int append){
    if(!f||f->type!=NODE_FILE) return -1;
    if(!append){
        if(aoff+len>sizeof(arena)) return -2;
        f->data=&arena[aoff];
        kmemcpy(f->data,data,len);
        f->size=len;
        aoff+=len;
        return 0;
    } else {
        size_t newsize = f->size + len;
        if(aoff+newsize>sizeof(arena)) return -2;
        char* nd=&arena[aoff];
        if(f->size) kmemcpy(nd, f->data, f->size);
        kmemcpy(nd + f->size, data, len);
        f->data = nd;
        f->size = newsize;
        aoff += newsize;
        return 0;
    }
}

int fs_read(node_t* f, char* out, size_t max){ if(!f||f->type!=NODE_FILE||!f->data) return 0; size_t n=f->size<max?f->size:max; kmemcpy(out,f->data,n); return (int)n; }

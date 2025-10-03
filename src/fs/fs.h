#pragma once
#include <stddef.h>

typedef enum { NODE_DIR, NODE_FILE } node_type_t;

typedef struct node {
    char name[32];
    node_type_t type;
    struct node* parent;
    struct node* sibling;
    struct node* child;
    char* data;
    size_t size;
} node_t;

void fs_init(void);
node_t* fs_root(void);
node_t* fs_cwd(void);
void fs_set_cwd(node_t* n);
node_t* fs_mkdir(node_t* parent, const char* name);
node_t* fs_lookup(node_t* parent, const char* path);
node_t* fs_create_file(node_t* parent, const char* name);
int fs_write(node_t* f, const char* data, size_t len, int append);
int fs_read(node_t* f, char* out, size_t max);

#include "fs.h"
#include "../kernel/kprint.h"
#include "../kernel/string.h"
#include "ata.h"

/* Simple disk-based filesystem with block allocation */
#define BLOCK_SIZE 512
#define MAX_INODES 512
#define MAX_DATA_BLOCKS 4096
#define INODE_START_BLOCK 1
#define DATA_START_BLOCK (1 + (MAX_INODES * sizeof(disk_inode_t) + BLOCK_SIZE - 1) / BLOCK_SIZE)

typedef struct {
    char name[32];
    uint8_t type;  // 0=unused, 1=file, 2=dir, 3=chardev
    uint32_t size;
    uint32_t parent_ino;
    uint32_t first_block;  // First data block
    uint32_t blocks_used;  // Number of blocks
} disk_inode_t;

/* In-memory cache */
static node_t d_root;
static node_t *d_cwd;
static node_t d_nodes[MAX_INODES];
static int d_ni = 0;
static char d_arena[131072];  // Temporary arena for file data
static size_t d_aoff = 0;
static int disk_initialized = 0;

static node_t *d_add_child(node_t *p, node_t *n)
{
    n->parent = p;
    n->sibling = p->child;
    n->child = 0;
    p->child = n;
    return n;
}

static node_t *d_find_in(node_t *p, const char *name)
{
    for (node_t *c = p->child; c; c = c->sibling)
        if (kstrcmp(c->name, name) == 0)
            return c;
    return 0;
}

/* Try to initialize disk, fall back to memory if it fails */
static int try_disk_init(void)
{
    char buf[BLOCK_SIZE];
    
    /* Try to read sector 0 (superblock) */
    if (ata_read28(0, buf) != 0) {
        kprintf("[fs-disk] ATA read failed, using memory-only mode\n");
        return 0;
    }
    
    /* Check for magic signature */
    if (buf[0] == 'S' && buf[1] == 'N' && buf[2] == 'O' && buf[3] == 'W') {
        kprintf("[fs-disk] Found SNOW filesystem on disk\n");
        disk_initialized = 1;
        return 1;
    }
    
    kprintf("[fs-disk] No filesystem found, creating new one\n");
    
    /* Initialize superblock */
    kmemset(buf, 0, BLOCK_SIZE);
    buf[0] = 'S'; buf[1] = 'N'; buf[2] = 'O'; buf[3] = 'W';
    if (ata_write28(0, buf) != 0) {
        kprintf("[fs-disk] Failed to write superblock, using memory-only\n");
        return 0;
    }
    
    /* Clear inode table */
    kmemset(buf, 0, BLOCK_SIZE);
    for (int i = 0; i < (MAX_INODES * sizeof(disk_inode_t) + BLOCK_SIZE - 1) / BLOCK_SIZE; i++) {
        if (ata_write28(INODE_START_BLOCK + i, buf) != 0) {
            kprintf("[fs-disk] Failed to initialize inode table\n");
            return 0;
        }
    }
    
    disk_initialized = 1;
    kprintf("[fs-disk] Disk filesystem initialized\n");
    return 1;
}

void fs_init(void)
{
    kprintf("[fs-disk] Initializing disk-backed filesystem\n");
    
    /* Initialize in-memory structures */
    kmemset(&d_root, 0, sizeof(d_root));
    d_root.name[0] = '/';
    d_root.name[1] = 0;
    d_root.type = NODE_DIR;
    d_cwd = &d_root;
    
    for (int i = 0; i < MAX_INODES; i++) {
        d_nodes[i].type = NODE_FILE;  // Will be set properly when used
        d_nodes[i].parent = 0;
        d_nodes[i].sibling = 0;
        d_nodes[i].child = 0;
        d_nodes[i].data = 0;
        d_nodes[i].size = 0;
    }
    
    /* Try to initialize disk */
    try_disk_init();
    
    if (!disk_initialized) {
        kprintf("[fs-disk] Running in memory-only mode (disk unavailable)\n");
    }
}

node_t *fs_root(void) { return &d_root; }
node_t *fs_cwd(void) { return d_cwd; }

void fs_set_cwd(node_t *n)
{
    if (n && n->type == NODE_DIR)
        d_cwd = n;
}

node_t *fs_mkdir(node_t *parent, const char *name)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    if (d_find_in(parent, name))
        return 0;
    if (d_ni >= MAX_INODES)
        return 0;
    
    node_t *n = &d_nodes[d_ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, name, 31);
    n->type = NODE_DIR;
    
    /* TODO: Write to disk if disk_initialized */
    
    return d_add_child(parent, n);
}

node_t *fs_create_file(node_t *parent, const char *name)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    
    node_t *e = d_find_in(parent, name);
    if (e) {
        if (e->type == NODE_FILE)
            return e;
        else
            return 0;
    }
    
    if (d_ni >= MAX_INODES)
        return 0;
    
    node_t *n = &d_nodes[d_ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, name, 31);
    n->type = NODE_FILE;
    
    /* TODO: Write to disk if disk_initialized */
    
    return d_add_child(parent, n);
}

node_t *fs_create_chardev(node_t *parent, const char *name, void *devptr)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    
    node_t *e = d_find_in(parent, name);
    if (e) return e;
    
    if (d_ni >= MAX_INODES)
        return 0;
    
    node_t *n = &d_nodes[d_ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, name, 31);
    n->type = NODE_CHAR;
    n->data = devptr;
    
    return d_add_child(parent, n);
}

node_t *fs_lookup(node_t *parent, const char *path)
{
    if (!path || !*path)
        return parent;
    if (path[0] == '/')
        parent = &d_root;
    
    char tmp[128];
    kstrncpy(tmp, path, 127);
    char *s = tmp;
    node_t *cur = parent;
    char *tok = s;
    
    for (size_t i = 0;; ++i) {
        if (s[i] == '/' || s[i] == '\0') {
            char c = s[i];
            s[i] = '\0';
            
            if (kstrcmp(tok, "") != 0) {
                if (kstrcmp(tok, ".") == 0) {
                    /* Stay in current directory */
                } else if (kstrcmp(tok, "..") == 0) {
                    if (cur->parent)
                        cur = cur->parent;
                } else {
                    node_t *f = d_find_in(cur, tok);
                    if (!f)
                        return 0;
                    cur = f;
                }
            }
            
            if (c == '\0')
                break;
            tok = &s[i + 1];
        }
    }
    return cur;
}

int fs_write(node_t *f, const char *data, size_t len, int append)
{
    if (!f || f->type != NODE_FILE)
        return -1;
    
    /* For now, use memory arena */
    if (!append) {
        if (d_aoff + len > sizeof(d_arena))
            return -2;
        f->data = &d_arena[d_aoff];
        kmemcpy(f->data, data, len);
        f->size = len;
        d_aoff += len;
        
        /* TODO: Write to disk if disk_initialized */
        
        return 0;
    } else {
        size_t newsize = f->size + len;
        if (d_aoff + newsize > sizeof(d_arena))
            return -2;
        
        char *nd = &d_arena[d_aoff];
        if (f->size)
            kmemcpy(nd, f->data, f->size);
        kmemcpy(nd + f->size, data, len);
        f->data = nd;
        f->size = newsize;
        d_aoff += newsize;
        
        /* TODO: Write to disk if disk_initialized */
        
        return 0;
    }
}

int fs_read(node_t *f, char *out, size_t max)
{
    if (!f || f->type != NODE_FILE || !f->data)
        return 0;
    
    size_t n = f->size < max ? f->size : max;
    kmemcpy(out, f->data, n);
    return (int)n;
}

node_t *fs_find_child(node_t *parent, const char *name)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    return d_find_in(parent, name);
}

node_t *fs_unlink(node_t *parent, const char *name)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    
    node_t *prev = 0;
    for (node_t *c = parent->child; c; prev = c, c = c->sibling) {
        if (kstrcmp(c->name, name) == 0) {
            if (prev)
                prev->sibling = c->sibling;
            else
                parent->child = c->sibling;
            c->parent = 0;
            c->sibling = 0;
            
            /* TODO: Free disk blocks if disk_initialized */
            
            return c;
        }
    }
    return 0;
}

int fs_rename(node_t *parent, const char *oldn, const char *newn)
{
    node_t *n = fs_find_child(parent, oldn);
    if (!n)
        return -1;
    if (d_find_in(parent, newn))
        return -2;
    
    kstrncpy(n->name, newn, 31);
    
    /* TODO: Update disk if disk_initialized */
    
    return 0;
}

node_t *fs_clone_file(node_t *parent, node_t *src, const char *newname)
{
    if (!parent || parent->type != NODE_DIR || !src || src->type != NODE_FILE)
        return 0;
    if (d_find_in(parent, newname))
        return 0;
    if (d_ni >= MAX_INODES)
        return 0;
    
    node_t *n = &d_nodes[d_ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, newname, 31);
    n->type = NODE_FILE;
    
    if (src->size) {
        if (d_aoff + src->size > sizeof(d_arena))
            return 0;
        n->data = &d_arena[d_aoff];
        kmemcpy(n->data, src->data, src->size);
        n->size = src->size;
        d_aoff += src->size;
    }
    
    d_add_child(parent, n);
    
    /* TODO: Write to disk if disk_initialized */
    
    return n;
}

void fs_stats(size_t *out_nodes, size_t *out_arena_used, size_t *out_arena_total)
{
    if (out_nodes)
        *out_nodes = (size_t)d_ni;
    if (out_arena_used)
        *out_arena_used = d_aoff;
    if (out_arena_total)
        *out_arena_total = sizeof(d_arena);
}


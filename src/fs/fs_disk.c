#include "fs.h"
#include "../kernel/kprint.h"
#include "../kernel/string.h"

void fs_mem_init(void);
extern void fs_init(void); // original symbol; we will wrap
extern node_t *fs_root(void);
extern node_t *fs_cwd(void);
extern void fs_set_cwd(node_t *n);
extern node_t *fs_mkdir(node_t *parent, const char *name);
extern node_t *fs_lookup(node_t *parent, const char *path);
extern node_t *fs_create_file(node_t *parent, const char *name);
extern int fs_write(node_t *f, const char *data, size_t len, int append);
extern int fs_read(node_t *f, char *out, size_t max);
extern node_t *fs_find_child(node_t *parent, const char *name);
extern node_t *fs_unlink(node_t *parent, const char *name);
extern int fs_rename(node_t *parent, const char *oldn, const char *newn);
extern node_t *fs_clone_file(node_t *parent, node_t *src, const char *newname);
extern void fs_stats(size_t *out_nodes, size_t *out_arena_used, size_t *out_arena_total);

typedef struct node node_t; // already in header, forward clarity

static node_t d_root; // disk fs root (in-memory mirror)
static node_t *d_cwd;
static node_t d_nodes[512];
static int d_ni = 0;
static char d_arena[131072];
static size_t d_aoff = 0;

static node_t *d_add_child(node_t *p, node_t *n)
{
    n->parent = p;
    n->sibling = p->child;
    n->child = 0;
    n->data = 0;
    n->size = 0;
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

void fs_init(void)
{
    kprintf("[fs-disk] transitional disk backend (embedded mem layer)\n");
    kmemset(&d_root, 0, sizeof(d_root));
    d_root.name[0] = '/';
    d_root.name[1] = 0;
    d_root.type = NODE_DIR;
    d_cwd = &d_root;
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
    if (d_ni >= 512)
        return 0;
    node_t *n = &d_nodes[d_ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, name, 31);
    n->type = NODE_DIR;
    return d_add_child(parent, n);
}
node_t *fs_create_file(node_t *parent, const char *name)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    node_t *e = d_find_in(parent, name);
    if (e)
    {
        if (e->type == NODE_FILE)
            return e;
        else
            return 0;
    }
    if (d_ni >= 512)
        return 0;
    node_t *n = &d_nodes[d_ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, name, 31);
    n->type = NODE_FILE;
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
    for (size_t i = 0;; ++i)
    {
        if (s[i] == '/' || s[i] == '\0')
        {
            char c = s[i];
            s[i] = '\0';
            if (kstrcmp(tok, "") != 0)
            {
                if (kstrcmp(tok, ".") == 0)
                {
                }
                else if (kstrcmp(tok, "..") == 0)
                {
                    if (cur->parent)
                        cur = cur->parent;
                }
                else
                {
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
    if (!append)
    {
        if (d_aoff + len > sizeof(d_arena))
            return -2;
        f->data = &d_arena[d_aoff];
        kmemcpy(f->data, data, len);
        f->size = len;
        d_aoff += len;
        return 0;
    }
    else
    {
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
    for (node_t *c = parent->child; c; prev = c, c = c->sibling)
    {
        if (kstrcmp(c->name, name) == 0)
        {
            if (prev)
                prev->sibling = c->sibling;
            else
                parent->child = c->sibling;
            c->parent = 0;
            c->sibling = 0;
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
    return 0;
}
node_t *fs_clone_file(node_t *parent, node_t *src, const char *newname)
{
    if (!parent || parent->type != NODE_DIR || !src || src->type != NODE_FILE)
        return 0;
    if (d_find_in(parent, newname))
        return 0;
    if (d_ni >= 512)
        return 0;
    node_t *n = &d_nodes[d_ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, newname, 31);
    n->type = NODE_FILE;
    if (src->size)
    {
        if (d_aoff + src->size > sizeof(d_arena))
            return 0;
        n->data = &d_arena[d_aoff];
        kmemcpy(n->data, src->data, src->size);
        n->size = src->size;
        d_aoff += src->size;
    }
    d_add_child(parent, n);
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

#include "fs.h"
#include "../kernel/string.h"
#include <stdint.h>
#include <stddef.h>
#ifndef FS_VERBOSE
#define FS_VERBOSE 0
#endif
#if FS_VERBOSE
#define FS_LOG(...) kprintf(__VA_ARGS__)
#else
#define FS_LOG(...) \
    do              \
    {               \
    } while (0)
#endif

static node_t root;
static node_t *cwd;
static node_t nodes[512];
static int ni = 0;
static char arena[131072];
static size_t aoff = 0;

static node_t *add_child(node_t *p, node_t *n)
{
    n->parent = p;
    n->sibling = p->child;
    n->child = 0;
    n->data = 0;
    n->size = 0;
    p->child = n;
    return n;
}

static node_t *find_in(node_t *p, const char *name)
{
    for (node_t *c = p->child; c; c = c->sibling)
        if (kstrcmp(c->name, name) == 0)
            return c;
    return 0;
}

void fs_init(void)
{
    extern void kprintf(const char *, ...);
    FS_LOG("[fs] fs_init: entry\n");
    FS_LOG("[fs] fs_init: root=0x%x nodes=0x%x arena=0x%x aoff=0x%x size=%u\n",
           (unsigned)(uintptr_t)&root, (unsigned)(uintptr_t)nodes, (unsigned)(uintptr_t)arena, (unsigned)(uintptr_t)&aoff, (unsigned)sizeof(root));
    FS_LOG("[fs] fs_init: about to zero root (%u bytes)\n", (unsigned)sizeof(root));
    unsigned char *_p = (unsigned char *)&root;
    for (size_t _i = 0; _i < sizeof(root); ++_i)
    {
        _p[_i] = 0;
        if (_i == 0)
            FS_LOG("[fs] fs_init: zeroed first byte\n");
    }
    FS_LOG("[fs] fs_init: zeroed all bytes\n");
    FS_LOG("[fs] fs_init: about to copy name\n");
    kstrcpy(root.name, "/");
    FS_LOG("[fs] fs_init: name copied -> %s\n", root.name);
    FS_LOG("[fs] fs_init: about to set fields\n");
    root.type = NODE_DIR;
    root.parent = 0;
    root.child = 0;
    root.sibling = 0;
    FS_LOG("[fs] fs_init: fields set\n");
    FS_LOG("[fs] fs_init: about to set cwd\n");
    cwd = &root;
    FS_LOG("[fs] fs_init: cwd set\n");
    FS_LOG("[fs] fs_init: done\n");
}

node_t *fs_root(void) { return &root; }
node_t *fs_cwd(void) { return cwd; }
void fs_set_cwd(node_t *n)
{
    if (n && n->type == NODE_DIR)
        cwd = n;
}

node_t *fs_mkdir(node_t *parent, const char *name)
{
    extern void kprintf(const char *, ...);
    FS_LOG("[fs] fs_mkdir: parent=0x%x name=%s ni=%u\n", (unsigned)(uintptr_t)parent, name ? name : "(null)", (unsigned)ni);
    if (!parent || parent->type != NODE_DIR)
    {
        FS_LOG("[fs] fs_mkdir: invalid parent\n");
        return 0;
    }
    if (find_in(parent, name))
    {
        FS_LOG("[fs] fs_mkdir: already exists\n");
        return 0;
    }
    if (ni >= 512)
    {
        FS_LOG("[fs] fs_mkdir: node table full\n");
        return 0;
    }
    node_t *n = &nodes[ni++];
    FS_LOG("[fs] fs_mkdir: allocated node 0x%x (ni now %u)\n", (unsigned)(uintptr_t)n, (unsigned)ni);
    for (size_t i = 0; i < sizeof(*n); ++i)
        ((unsigned char *)n)[i] = 0;
    if (name)
    {
        size_t j = 0;
        for (; j < 31 && name[j]; ++j)
            n->name[j] = name[j];
        n->name[j] = 0;
    }
    else
    {
        n->name[0] = 0;
    }
    n->type = NODE_DIR;
    n->data = 0;
    n->size = 0;
    n->parent = parent;
    n->sibling = parent->child;
    parent->child = n;
    FS_LOG("[fs] fs_mkdir: parent->child now 0x%x\n", (unsigned)(uintptr_t)parent->child);
    return n;
}

node_t *fs_create_file(node_t *parent, const char *name)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    node_t *e = find_in(parent, name);
    if (e)
    {
        if (e->type == NODE_FILE)
            return e;
        else
            return 0;
    }
    if (ni >= 512)
        return 0;
    node_t *n = &nodes[ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, name, 31);
    n->type = NODE_FILE;
    return add_child(parent, n);
}

node_t *fs_create_chardev(node_t *parent, const char *name, void *devptr)
{
    if (!parent || parent->type != NODE_DIR)
        return 0;
    node_t *e = find_in(parent, name);
    if (e)
    {
        if (e->type == NODE_CHAR)
            return e;
        else
            return 0;
    }
    if (ni >= 512)
        return 0;
    node_t *n = &nodes[ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, name, 31);
    n->type = NODE_CHAR;
    n->data = (char*)devptr;
    n->size = 0;
    return add_child(parent, n);
}

node_t *fs_lookup(node_t *parent, const char *path)
{
    if (!path || !*path)
        return parent;
    if (path[0] == '/')
        parent = &root;
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
                    node_t *f = find_in(cur, tok);
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
        if (aoff + len > sizeof(arena)) {
            f->data = (char*)data;
            f->size = len;
            return 0;
        }
        f->data = &arena[aoff];
        kmemcpy(f->data, data, len);
        f->size = len;
        aoff += len;
        return 0;
    }
    else
    {
        size_t newsize = f->size + len;
        if (aoff + newsize > sizeof(arena))
            return -2;
        char *nd = &arena[aoff];
        if (f->size)
            kmemcpy(nd, f->data, f->size);
        kmemcpy(nd + f->size, data, len);
        f->data = nd;
        f->size = newsize;
        aoff += newsize;
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
    return find_in(parent, name);
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
    if (find_in(parent, newn))
        return -2;
    kstrncpy(n->name, newn, 31);
    return 0;
}

node_t *fs_clone_file(node_t *parent, node_t *src, const char *newname)
{
    if (!parent || parent->type != NODE_DIR || !src || src->type != NODE_FILE)
        return 0;
    if (find_in(parent, newname))
        return 0;
    if (ni >= 512)
        return 0;
    node_t *n = &nodes[ni++];
    kmemset(n, 0, sizeof(*n));
    kstrncpy(n->name, newname, 31);
    n->type = NODE_FILE;
    if (src->size)
    {
        if (aoff + src->size > sizeof(arena))
            return 0;
        n->data = &arena[aoff];
        kmemcpy(n->data, src->data, src->size);
        n->size = src->size;
        aoff += src->size;
    }
    add_child(parent, n);
    return n;
}

void fs_stats(size_t *out_nodes, size_t *out_arena_used, size_t *out_arena_total)
{
    if (out_nodes)
        *out_nodes = (size_t)ni;
    if (out_arena_used)
        *out_arena_used = aoff;
    if (out_arena_total)
        *out_arena_total = sizeof(arena);
}

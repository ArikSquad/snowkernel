#include "shell.h"
#include "kprint.h"
#include "keyboard.h"
#include "serial.h"
#include "../fs/fs.h"
#include "string.h"
#include "env.h"

typedef void (*cmd_fn)(char *);
typedef struct
{
    const char *name;
    const char *desc;
    cmd_fn fn;
} cmd_t;

static char redir_buf[4096];
static unsigned redir_len = 0;
static void redir_sink_char(char c)
{
    if (redir_len < sizeof(redir_buf))
        redir_buf[redir_len++] = c;
}

static void get_cwd_path(char *out, int max)
{
    node_t *stack[32];
    int sp = 0;
    node_t *cur = fs_cwd();
    while (cur)
    {
        stack[sp++] = cur;
        cur = cur->parent;
        if (sp >= 32)
            break;
    }
    int j = 0;
    if (sp == 1)
    {
        out[0] = '/';
        out[1] = '\0';
        return;
    }
    for (int i = sp - 1; i >= 0; i--)
    {
        if (i == sp - 1)
            continue;
        out[j++] = '/';
        char *n = stack[i]->name;
        for (int k = 0; n[k] && j < max - 1; k++)
            out[j++] = n[k];
        if (j >= max - 1)
            break;
    }
    out[j] = '\0';
}

static void prompt()
{
    char p[128];
    get_cwd_path(p, 128);
    kprintf("%s $ ", p);
}

static void builtin_help(char *);
static void builtin_clear(char *);
static void builtin_echo(char *);
static void builtin_cat(char *);
static void builtin_mkdir(char *);
static void builtin_cd(char *);
static void builtin_ls(char *);
static void builtin_touch(char *);
static void builtin_rm(char *);
static void builtin_mv(char *);
static void builtin_cp(char *);
static void builtin_hw(char *);
static void builtin_free(char *);
static void builtin_pwd(char *);
static void builtin_keymap(char *);
static void builtin_edit(char *);
static void builtin_env(char *);
static void builtin_set(char *);
static void builtin_unset(char *);

static cmd_t CMDS[] = {
    {"help", "List commands", builtin_help},
    {"clear", "Clear screen", builtin_clear},
    {"echo", "Print text", builtin_echo},
    {"cat", "Show file contents", builtin_cat},
    {"mkdir", "Create directory", builtin_mkdir},
    {"cd", "Change directory", builtin_cd},
    {"ls", "List directory", builtin_ls},
    {"touch", "Create empty file", builtin_touch},
    {"rm", "Remove file or empty dir", builtin_rm},
    {"mv", "Rename (same dir)", builtin_mv},
    {"cp", "Copy file", builtin_cp},
    {"pwd", "Print working directory", builtin_pwd},
    {"keymap", "Get/Set keymap", builtin_keymap},
    {"edit", "Edit file", builtin_edit},
    {"env", "List environment", builtin_env},
    {"set", "Set NAME=VALUE", builtin_set},
    {"unset", "Unset NAME", builtin_unset},
    {"hw", "Kernel info", builtin_hw},
    {"free", "Memory usage", builtin_free},
};
static const int CMDS_N = sizeof(CMDS) / sizeof(CMDS[0]);

static void builtin_help(char *args)
{
    (void)args;
    for (int i = 0; i < CMDS_N; i++)
    {
        kprintf("%s - %s\n", CMDS[i].name, CMDS[i].desc);
    }
}
static void builtin_clear(char *args)
{
    (void)args;
    kclear();
}

static int parse_redir(char *line, char **left, char **file, int *append)
{
    *append = 0;
    *file = 0;
    *left = line;
    char *p = kstrchr(line, '>');
    if (!p)
        return 0;
    *p = '\0';
    p++;
    if (*p == '>')
    {
        *append = 1;
        p++;
    }
    while (*p == ' ')
        p++;
    if (*p == '\0')
        return 0;
    *file = p;
    return 1;
}

static void builtin_echo(char *args)
{
    kputs(args);
    kputc('\n');
}

static void builtin_cat(char *args)
{
    node_t *n = fs_lookup(fs_cwd(), args);
    if (!n || n->type != NODE_FILE)
    {
        kputs("not a file\n");
        return;
    }
    char buf[512];
    int r = fs_read(n, buf, sizeof(buf) - 1);
    buf[r] = 0;
    kputs(buf);
    kputc('\n');
}

static void builtin_mkdir(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: mkdir <name>\n");
        return;
    }
    if (!fs_mkdir(fs_cwd(), args))
        kputs("error\n");
}
static void builtin_cd(char *args)
{
    node_t *n = fs_lookup(fs_cwd(), args);
    if (n && n->type == NODE_DIR)
        fs_set_cwd(n);
    else
        kputs("no such dir\n");
}

static void builtin_ls(char *args)
{
    (void)args;
    node_t *cwd = fs_cwd();
    for (node_t *c = cwd->child; c; c = c->sibling)
    {
        kprintf("%s%s\n", c->name, c->type == NODE_DIR ? "/" : "");
    }
}

static void builtin_touch(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: touch <name>\n");
        return;
    }
    if (!fs_create_file(fs_cwd(), args))
        kputs("error\n");
}

static void builtin_rm(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: rm <name>\n");
        return;
    }
    node_t *cwd = fs_cwd();
    node_t *n = fs_find_child(cwd, args);
    if (!n)
    {
        kputs("no such entry\n");
        return;
    }
    if (n->type == NODE_DIR && n->child)
    {
        kputs("not empty\n");
        return;
    }
    if (!fs_unlink(cwd, args))
        kputs("fail\n");
}

static void builtin_mv(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: mv <old> <new>\n");
        return;
    }
    char *b = args;
    while (*b && *b != ' ')
        b++;
    if (!*b)
    {
        kputs("usage: mv <old> <new>\n");
        return;
    }
    *b = '\0';
    b++;
    while (*b == ' ')
        b++;
    if (!*b)
    {
        kputs("usage: mv <old> <new>\n");
        return;
    }
    int r = fs_rename(fs_cwd(), args, b);
    if (r == -1)
        kputs("no such entry\n");
    else if (r == -2)
        kputs("target exists\n");
}

static void builtin_cp(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: cp <src> <dst>\n");
        return;
    }
    char *b = args;
    while (*b && *b != ' ')
        b++;
    if (!*b)
    {
        kputs("usage: cp <src> <dst>\n");
        return;
    }
    *b = '\0';
    b++;
    while (*b == ' ')
        b++;
    if (!*b)
    {
        kputs("usage: cp <src> <dst>\n");
        return;
    }
    node_t *src = fs_find_child(fs_cwd(), args);
    if (!src || src->type != NODE_FILE)
    {
        kputs("no such file\n");
        return;
    }
    if (!fs_clone_file(fs_cwd(), src, b))
        kputs("error\n");
}

static void builtin_hw(char *args)
{
    (void)args;
    kprintf("kernel: SnowKernel 0.1\narch: i386\nvideo: text %dx%d\n", 80, 25);
}

static void builtin_free(char *args)
{
    (void)args;
    size_t nodes, used, tot;
    fs_stats(&nodes, &used, &tot);
    kprintf("nodes: %u used: %u arena: %u/%u\n", (unsigned)nodes, (unsigned)(nodes * sizeof(node_t)), (unsigned)used, (unsigned)tot);
}

static void builtin_pwd(char *args)
{
    (void)args;
    char p[128];
    get_cwd_path(p, 128);
    kprintf("%s\n", p);
}

static void builtin_keymap(char *args)
{
    if (!args || !*args)
    {
        kprintf("keymap: %s\n", kbd_get_keymap() == 1 ? "dvorak" : (kbd_get_keymap() == 2 ? "fi" : "us"));
        return;
    }
    if (kstrcmp(args, "us") == 0)
        kbd_set_keymap(0);
    else if (kstrcmp(args, "dvorak") == 0)
        kbd_set_keymap(1);
    else if (kstrcmp(args, "fi") == 0)
        kbd_set_keymap(2);
    else
        kputs("usage: keymap [us|dvorak|fi]\n");
}

static void builtin_edit(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: edit <file>\n");
        return;
    }
    node_t *f = fs_create_file(fs_cwd(), args);
    if (!f)
    {
        kputs("cannot open\n");
        return;
    }
    kputs("-- editing ('.' alone to save, '!q' to abort) --\n");
    char line[256];
    size_t total = 0;
    char tmpbuf[4096];
    for (;;)
    {
        int len = kbd_read_line(line, sizeof(line));
        if (len == 0 && line[0] == 0)
            continue;
        if (kstrcmp(line, "!q") == 0)
        {
            kputs("(aborted)\n");
            return;
        }
        if (kstrcmp(line, ".") == 0)
        {
            fs_write(f, tmpbuf, total, 0);
            kputs("(saved)\n");
            return;
        }
        if (total + len + 1 >= sizeof(tmpbuf))
        {
            kputs("(buffer full)\n");
            fs_write(f, tmpbuf, total, 0);
            return;
        }
        for (int i = 0; i < len; i++)
            tmpbuf[total++] = line[i];
        tmpbuf[total++] = '\n';
    }
}

static int read_line_mux(char *buf, int max)
{
    int i = 0;
    for (;;)
    {
        int v = serial_getc_nonblock();
        if (v >= 0)
        {
            char c = (char)v;
            if (c == '\r')
                c = '\n';
            if (c == '\n')
            {
                kputc('\n');
                buf[i] = 0;
                return i;
            }
            if ((c == 8 || c == 127))
            {
                if (i > 0)
                {
                    i--;
                    kputc('\b');
                    kputc(' ');
                    kputc('\b');
                }
            }
            else if (i < max - 1)
            {
                buf[i++] = c;
                kputc(c);
            }
        }
        v = kbd_getc_nonblock();
        if (v >= 0)
        {
            char c = (char)v;
            if (c == '\n')
            {
                kputc('\n');
                buf[i] = 0;
                return i;
            }
            if (c == 8)
            {
                if (i > 0)
                {
                    i--;
                    kputc('\b');
                    kputc(' ');
                    kputc('\b');
                }
            }
            else if (i < max - 1)
            {
                buf[i++] = c;
                kputc(c);
            }
        }
    }
}

void shell_run(void)
{
    char line[256];
    for (;;)
    {
        prompt();
        int n = read_line_mux(line, sizeof(line));
        if (n <= 0)
            continue;
        char *file = 0;
        int append = 0;
        char *left = line;
        parse_redir(line, &left, &file, &append);
        char *cmd = left;
        while (*cmd == ' ')
            cmd++;
        char *args = cmd;
        while (*args && *args != ' ')
            args++;
        if (*args)
        {
            *args = '\0';
            args++;
            while (*args == ' ')
                args++;
        }
        int handled = 0;
        for (int i = 0; i < CMDS_N; i++)
            if (kstrcmp(cmd, CMDS[i].name) == 0)
            {
                if (file)
                {
                    node_t *f = fs_create_file(fs_cwd(), file);
                    if (!f)
                    {
                        kputs("redir error\n");
                        handled = 1;
                        break;
                    }
                    redir_len = 0;
                    kputc_fn prev = kprint_set_sink(redir_sink_char);
                    CMDS[i].fn(args && *args ? args : (char *)"");
                    kprint_set_sink(prev);
                    fs_write(f, redir_buf, redir_len, append);
                }
                else
                {
                    CMDS[i].fn(args && *args ? args : (char *)"");
                }
                handled = 1;
                break;
            }
        if (!handled)
            kputs("This command does not exist\n");
    }
}

static void env_list_sink(const char *kv) { kprintf("%s\n", kv); }
static void builtin_env(char *args)
{
    (void)args;
    env_list(env_list_sink);
}
static void builtin_set(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: set NAME=VALUE\n");
        return;
    }
    if (env_set(args) != 0)
        kputs("set failed\n");
    else
        env_save();
}
static void builtin_unset(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: unset NAME\n");
        return;
    }
    if (env_unset(args) != 0)
        kputs("unset failed\n");
    else
        env_save();
}

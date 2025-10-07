#include "shell.h"
#include "kprint.h"
#include "../drivers/keyboard.h"
#include "serial.h"
#include "../fs/fs.h"
#include "string.h"
#include "env.h"
#include "syscall.h"

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
static void builtin_exit(char *arg);
static void builtin_export(char *arg);
static void builtin_ui(char *args);

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
    {"exit", "Exit shell", builtin_exit},
    {"export", "Export NAME=VALUE", builtin_export},
    {"hw", "Kernel info", builtin_hw},
    {"free", "Memory usage", builtin_free},
    {"ui", "Launch simple UI", builtin_ui},
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

// --- New tokenizer and command runner ---

typedef struct
{
    char *argv[32];
    int argc;
    const char *out_redir;
    int out_append;
    const char *in_redir;
} parsed_cmd_t;

static void parse_command_line(char *line, parsed_cmd_t *pc)
{
    pc->argc = 0;
    pc->out_redir = 0;
    pc->out_append = 0;
    pc->in_redir = 0;
    // Detect redirections
    char *p = line;
    char *last_space = 0;
    while (*p)
    {
        if (*p == ' ')
            last_space = p;
        if (*p == '<')
        {
            *p = '\0';
            char *f = p + 1;
            while (*f == ' ')
                f++;
            pc->in_redir = f;
            break;
        }
        if (*p == '>')
        {
            int append = 0;
            if (p[1] == '>')
            {
                append = 1;
                p++;
            }
            *p = '\0';
            char *f = p + 1;
            while (*f == ' ')
                f++;
            pc->out_redir = f;
            pc->out_append = append;
            break;
        }
        p++;
    }
    // If redirections found, use the part before
    char *cmd_part = line;
    if (pc->in_redir || pc->out_redir)
    {
        if (last_space)
            *last_space = '\0';
    }
    // Tokenize cmd_part respecting quotes
    p = cmd_part;
    while (*p && pc->argc < 31)
    {
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        char *start = p;
        int quote = 0;
        if (*p == '"' || *p == '\'')
        {
            quote = *p;
            start = ++p;
        }
        while (*p && ((quote && *p != quote) || (!quote && *p != ' ')))
            p++;
        if (quote && *p == quote)
        {
            *p = '\0';
            p++;
        }
        else if (!quote && *p)
        {
            *p = '\0';
            p++;
        }
        pc->argv[pc->argc++] = start;
    }
    pc->argv[pc->argc] = 0;
}

static int run_builtin(parsed_cmd_t *pc)
{
    if (pc->argc == 0)
        return 1; // nothing
    for (int i = 0; i < CMDS_N; i++)
    {
        if (kstrcmp(pc->argv[0], CMDS[i].name) == 0)
        {
            char args_buf[256];
            args_buf[0] = '\0';
            if (pc->argc > 1)
            {
                int pos = 0;
                for (int a = 1; a < pc->argc; a++)
                {
                    char *s = pc->argv[a];
                    for (int k = 0; s[k] && pos < (int)sizeof(args_buf) - 2; k++)
                        args_buf[pos++] = s[k];
                    if (a + 1 < pc->argc && pos < (int)sizeof(args_buf) - 1)
                        args_buf[pos++] = ' ';
                }
                args_buf[pos] = '\0';
            }
            if (pc->out_redir)
            {
                node_t *f = fs_create_file(fs_cwd(), pc->out_redir);
                if (!f)
                {
                    kputs("redir error\n");
                    return 1;
                }
                redir_len = 0;
                kputc_fn prev = kprint_set_sink(redir_sink_char);
                CMDS[i].fn(args_buf);
                kprint_set_sink(prev);
                fs_write(f, redir_buf, redir_len, pc->out_append);
            }
            else
            {
                CMDS[i].fn(pc->argc > 1 ? args_buf : (char *)"");
            }
            return 1;
        }
    }
    return 0;
}

extern long sys_execve(const char *path, char *const argv[], char *const envp[]);
static void run_external(parsed_cmd_t *pc)
{
    if (pc->argc == 0)
        return;
    int saved_stdout = -1;
    int saved_stdin = -1;
    if (pc->in_redir)
    {
        int fd = sys_open(pc->in_redir, 0, 0); // O_RDONLY
        if (fd < 0)
        {
            kputs("cannot open input file\n");
            return;
        }
        saved_stdin = sys_dup(0);
        sys_dup2(fd, 0);
        sys_close(fd);
    }
    if (pc->out_redir)
    {
        int fd = sys_open(pc->out_redir, 1 | 64 | (pc->out_append ? 1024 : 512), 0644); // O_WRONLY|O_CREAT|O_TRUNC/APPEND
        if (fd < 0)
        {
            kputs("cannot open output file\n");
            if (saved_stdin >= 0)
            {
                sys_dup2(saved_stdin, 0);
                sys_close(saved_stdin);
            }
            return;
        }
        saved_stdout = sys_dup(1);
        sys_dup2(fd, 1);
        sys_close(fd);
    }
    const char *path = env_get("PATH");
    if (!path || !*path)
        path = "/bin";
    char temp[128];
    const char *seg = path;
    const char *cur = path;
    int attempted = 0;
    while (1)
    {
        if (*cur == ':' || *cur == '\0')
        {
            size_t len = cur - seg;
            if (len == 0)
            {
                seg = cur + (*cur ? 1 : 0);
                cur++;
                if (!*cur)
                    break;
                continue;
            }
            if (len > sizeof(temp) - 1)
                len = sizeof(temp) - 1;
            for (size_t i = 0; i < len; i++)
                temp[i] = seg[i];
            temp[len] = '\0';
            // build candidate path temp + '/' + argv[0]
            size_t pos = len;
            if (pos && temp[pos - 1] != '/')
                temp[pos++] = '/';
            for (int k = 0; pc->argv[0][k] && pos < (int)sizeof(temp) - 1; k++)
                temp[pos++] = pc->argv[0][k];
            temp[pos] = '\0';
            // try execve
            long r = sys_execve(temp, pc->argv, 0);
            if (r >= 0)
            {
                attempted = 1;
                break;
            }
            if (*cur == '\0')
                break;
            seg = cur + 1;
        }
        if (*cur == '\0')
            break;
        else
            cur++;
    }
    if (!attempted)
    {
        kputs("command not found\n");
    }

    if (saved_stdout >= 0)
    {
        sys_dup2(saved_stdout, 1);
        sys_close(saved_stdout);
    }
    if (saved_stdin >= 0)
    {
        sys_dup2(saved_stdin, 0);
        sys_close(saved_stdin);
    }
}

void shell_run(void)
{
    char line[256];
    parsed_cmd_t pc;
    for (;;)
    {
        prompt();
        int n = read_line_mux(line, sizeof(line));
        if (n <= 0)
            continue;
        parse_command_line(line, &pc);
        if (pc.argc == 0)
            continue;
        if (run_builtin(&pc))
            continue;
        run_external(&pc);
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

static void builtin_exit(char *args)
{
    (void)args;
    sys_exit(0);
}

static void builtin_export(char *args)
{
    if (!args || !*args)
    {
        kputs("usage: export VAR=value\n");
        return;
    }
    char *eq = kstrchr(args, '=');
    if (!eq)
    {
        kputs("usage: export VAR=value\n");
        return;
    }
    *eq = '\0';
    char buf[256];
    kstrcpy(buf, args);
    kstrcat(buf, "=");
    kstrcat(buf, eq + 1);
    if (env_set(buf) != 0)
        kputs("export failed\n");
    else
        env_save();
}

extern void ui_run(void);
static void builtin_ui(char *args)
{
    (void)args;
    ui_run();
}

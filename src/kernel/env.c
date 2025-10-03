#include "env.h"
#include "kprint.h"
#include "string.h"
#include "ata.h"

#define ENV_MAX 64
#define ENV_KV_MAX 96
static char env_store[ENV_MAX][ENV_KV_MAX];
static int env_count = 0;
static int initialized = 0;

static int name_match(const char *kv, const char *name)
{
    for (int i = 0; kv[i] && kv[i] != '='; ++i)
        if (kv[i] != name[i])
            return 0;

    for (int i = 0; name[i]; ++i)
    {
        if (name[i] == '=')
            return 0; // invalid name
        if (kv[i] == '=')
            return 0; // kv shorter than name
    }

    int nlen = 0;
    while (name[nlen])
        nlen++;
    return kv[nlen] == '=';
}

void env_init(void)
{
    if (initialized)
        return;
    initialized = 1;

    // default path
    env_set("PATH=/bin");

    unsigned char sector[512];
    if (ata_read28(16, sector) == 0)
    {
        if (sector[0] == 'E' && sector[1] == 'N' && sector[2] == 'V' && sector[3] == '0')
        {
            int c = sector[4];
            int off = 5;
            for (int i = 0; i < c && i < ENV_MAX; i++)
            {
                if (off >= 512)
                    break;
                int len = sector[off++];
                if (len <= 0 || len >= ENV_KV_MAX)
                {
                    break;
                }
                if (off + len > 512)
                    break;
                char tmp[ENV_KV_MAX];
                for (int j = 0; j < len; j++)
                    tmp[j] = (char)sector[off + j];
                tmp[len] = 0;
                env_set(tmp);
                off += len;
            }
        }
    }
}

const char *env_get(const char *name)
{
    for (int i = 0; i < env_count; ++i)
        if (name_match(env_store[i], name))
        {
            const char *kv = env_store[i];
            while (*kv && *kv != '=')
                kv++;
            if (*kv == '=')
                return kv + 1;
            return "";
        }
    return 0;
}

int env_unset(const char *name)
{
    for (int i = 0; i < env_count; ++i)
    {
        if (name_match(env_store[i], name))
        {
            env_count--;
            if (i != env_count)
                kmemcpy(env_store[i], env_store[env_count], ENV_KV_MAX);
            return 0;
        }
    }
    return -1;
}

int env_set(const char *kv)
{
    if (!kv)
        return -1;
    int eq = -1;
    for (int i = 0; kv[i]; ++i)
    {
        if (kv[i] == '=')
        {
            eq = i;
            break;
        }
        if (i >= ENV_KV_MAX - 2)
            return -2;
    }

    if (eq <= 0)
        return -3; // need NAME=VALUE with non-empty name

    // replace if exists
    char name[64];
    if (eq >= (int)sizeof(name))
        return -4;
    for (int i = 0; i < eq; i++)
        name[i] = kv[i];
    name[eq] = 0;
    for (int i = 0; i < env_count; ++i)
    {
        if (name_match(env_store[i], name))
        {
            kstrncpy(env_store[i], kv, ENV_KV_MAX - 1);
            env_store[i][ENV_KV_MAX - 1] = 0;
            return 0;
        }
    }
    if (env_count >= ENV_MAX)
        return -5;
    kstrncpy(env_store[env_count], kv, ENV_KV_MAX - 1);
    env_store[env_count][ENV_KV_MAX - 1] = 0;
    env_count++;
    return 0;
}

void env_list(void (*cb)(const char *kv))
{
    for (int i = 0; i < env_count; ++i)
        cb(env_store[i]);
}

int env_save(void)
{
    unsigned char sector[512];
    for (int i = 0; i < 512; i++)
        sector[i] = 0;
    sector[0] = 'E';
    sector[1] = 'N';
    sector[2] = 'V';
    sector[3] = '0';
    sector[4] = (unsigned char)env_count;
    int off = 5;
    for (int i = 0; i < env_count && off < 512; i++)
    {
        const char *kv = env_store[i];
        int len = 0;
        while (kv[len] && len < ENV_KV_MAX - 1)
            len++;
        if (len > 250)
            len = 250; // prevent overflow
        if (off + 1 + len >= 512)
            break;
        sector[off++] = (unsigned char)len;
        for (int j = 0; j < len; j++)
            sector[off + j] = (unsigned char)kv[j];
        off += len;
    }
    return ata_write28(16, sector);
}

#pragma once
#include <stddef.h>

void env_init(void); 
int env_set(const char *kv); 
const char *env_get(const char *name);
int env_unset(const char *name);
void env_list(void (*cb)(const char *kv));
int env_save(void);

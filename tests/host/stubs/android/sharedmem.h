#pragma once
#include <stddef.h>
/* Host-test declaration only. Production builds link libandroid. */
int ASharedMemory_create(const char *name, size_t size);

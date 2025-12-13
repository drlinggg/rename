// Simple runtime debug flag & macro
#ifndef RENAME_DEBUG_H
#define RENAME_DEBUG_H

#include <stdio.h>

extern int debug_enabled;

#define DPRINT(fmt, ...) do { if (debug_enabled) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

#endif // RENAME_DEBUG_H

#pragma once
#include <cstdio>

#define MESSAGE_LEVEL 0

#if MESSAGE_LEVEL >= 0
#define info(fmt, ...) printf("I-" fmt, __VA_ARGS__)
#else
#define info(...)
#endif

#if MESSAGE_LEVEL >= 1
#define debug(fmt, ...) printf("D-" fmt, __VA_ARGS__)
#else
#define debug(...)
#endif
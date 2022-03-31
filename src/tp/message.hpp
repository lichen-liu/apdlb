#pragma once

#include <cstdio>

#define MESSAGE_LEVEL 2

#if MESSAGE_LEVEL >= 1
#define warn(fmt, ...) printf("W-" fmt, ##__VA_ARGS__)
#else
#define warn(...)
#endif

#if MESSAGE_LEVEL >= 2
#define info(fmt, ...) printf("I-" fmt, ##__VA_ARGS__)
#else
#define info(...)
#endif

#if MESSAGE_LEVEL >= 3
#define debug(fmt, ...) printf("D-" fmt, ##__VA_ARGS__)
#else
#define debug(...)
#endif
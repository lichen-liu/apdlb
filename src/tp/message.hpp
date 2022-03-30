#pragma once
#include <cstdio>

#define ENABLE_INFO

#ifdef ENABLE_INFO
#define info(...) printf(__VA_ARGS__)
#else
#define info(...)
#endif
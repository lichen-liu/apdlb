#pragma once

#include <cstdlib>
#include <cstdio>
#include <sys/time.h>

inline double get_time_stamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_usec / 1000000 + tv.tv_sec;
}

inline double print_elapsed(const char *prog, double start_time)
{
    const double end_time = get_time_stamp();
    const double elapsed = end_time - start_time;
    printf("\"%s\" Took %f seconds\n", prog, elapsed);
    return elapsed;
}

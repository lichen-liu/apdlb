#pragma once

namespace KBM
{
    inline int rand_r(int &seed)
    {
        seed = seed * 0x343fd + 0x269EC3; // a=214013, b=2531011
        return (seed >> 0x10) & 0x7FFF;
    }

    inline int fcomp(const void *a, const void *b)
    {
        const float a_val = *static_cast<const float *>(a);
        const float b_val = *static_cast<const float *>(b);
        if (a_val < b_val)
        {
            return -1;
        }
        else if (b_val < a_val)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}

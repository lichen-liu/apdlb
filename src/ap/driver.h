#pragma once

#include "rose.h"

namespace AutoParallelization
{
    void auto_parallize(SgProject *project, int target_nthreads, bool enable_debug);
}
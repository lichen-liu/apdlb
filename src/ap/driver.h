#pragma once

#include "rose.h"
#include "types.hpp"

namespace AutoParallelization
{
    void auto_parallize(SgProject *project, int target_nthreads, AP::ERT_TYPE ert_type, bool enable_debug);
}
#pragma once

#include "rose.h"

#include <vector>

namespace AP {
    std::vector<SgForStatement *> decideFinalLoopCandidates(const std::vector<SgForStatement *> &candidates);
    void insertERTIntoForLoop(SgForStatement *for_stmt);
}
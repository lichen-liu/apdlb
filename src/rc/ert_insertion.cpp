#include "ert_insertion.h"
#include "utils.h"

namespace RC
{
    void insertERTIntoForLoop(SgForStatement *for_stmt)
    {
        SageInterface::attachComment(for_stmt, "================ APERT ================");
        print_ast(for_stmt);
    }
}
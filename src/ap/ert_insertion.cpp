#include "ert_insertion.h"
#include "utils.h"

namespace AP
{
    // http://rosecompiler.org/ROSE_HTML_Reference/namespaceSageBuilder.html#a9c9bb07f0244282e95da666ed3947940
    void insertERTIntoForLoop(SgForStatement *for_stmt)
    {
        SageInterface::attachComment(for_stmt, "================ APERT ================");
        print_ast(for_stmt);
    }
}
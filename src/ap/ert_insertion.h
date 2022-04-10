#pragma once

#include "rose.h"

#include <string>
#include <vector>

namespace AP
{
    std::vector<SgForStatement *> decideFinalLoopCandidates(const std::vector<SgForStatement *> &candidates);

    class SourceFileERTInserter
    {
    public:
        explicit SourceFileERTInserter(SgSourceFile *sfile) : sfile_(sfile) {}
        ~SourceFileERTInserter();

        void insertERTIntoForLoop(SgForStatement *for_stmt);
        void insertERTIntoFunction(SgFunctionDefinition *defn);

        bool is_ert_used() const { return this->is_ert_used_; }

    private:
        void insertERTHeaderIntoSourceFile();

    private:
        SgSourceFile *sfile_ = nullptr;
        std::string ert_pool_type_include_ = "wspdr_pool.hpp";
        std::string ert_pool_type_ = "ERT::WSPDR_POOL";
        std::string ert_pool_name_ = "__apert_ert_pool";
        std::string ert_tasks_name_ = "__apert_ert_tasks";
        std::string ert_task_name_ = "__apert_ert_task";
        bool is_ert_used_ = false;
    };

}
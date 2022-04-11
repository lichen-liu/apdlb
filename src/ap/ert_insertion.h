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
        // -1 means let generated code decide num_threads at runtime
        void insertERTIntoFunction(SgFunctionDefinition *defn, int num_threads = -1);

        bool is_ert_used() const { return this->is_ert_used_; }

    private:
        void insertERTHeaderIntoSourceFile();

    private:
        SgSourceFile *sfile_ = nullptr;
        std::string ert_pool_type_include_header_ = "wspdr_pool.hpp";
        std::string ert_pool_type_ = "ERT::WSPDR_POOL";
        std::string ert_pool_name_ = "__apert_ert_pool";
        std::string ert_tasks_name_ = "__apert_ert_tasks";
        std::string ert_task_name_ = "__apert_ert_task";
        bool is_ert_used_ = false;
        bool should_include_thread_header_ = false;
    };

}
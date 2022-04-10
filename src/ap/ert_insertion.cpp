#include "ert_insertion.h"
#include "config.hpp"
#include "utils.h"

namespace AP
{
    std::vector<SgForStatement *> decideFinalLoopCandidates(const std::vector<SgForStatement *> &candidates)
    {
        if (AP::Config::get().enable_debug)
        {
            std::cout << std::endl;
            std::cout << "Determining final loop candidates for parallelization.." << std::endl;
        }
        std::vector<SgForStatement *> final_candidates;
        for (SgForStatement *cur_node : candidates)
        {
            bool not_descendent = std::all_of(candidates.begin(),
                                              candidates.end(),
                                              [cur_node](SgForStatement *target_node)
                                              {
                                                  return !SageInterface::isAncestor(target_node, cur_node);
                                              });
            if (not_descendent)
            {
                final_candidates.emplace_back(cur_node);
            }
        }
        if (AP::Config::get().enable_debug)
        {
            std::cout << "Loop candidates below are rejected because they are descendeds of another loop candidate:" << std::endl;
            std::vector<SgForStatement *> rejected_loop_candidates;
            std::set_difference(candidates.begin(), candidates.end(),
                                final_candidates.begin(), final_candidates.end(),
                                std::back_inserter(rejected_loop_candidates));
            for (SgForStatement *rejected_loop_candidate : rejected_loop_candidates)
            {
                std::cout << "  loop at line:" << rejected_loop_candidate->get_file_info()->get_line() << std::endl;
            }
        }

        return final_candidates;
    }

    SourceFileERTInserter::~SourceFileERTInserter()
    {
        if (this->is_ert_used())
        {
            this->insertERTHeaderIntoSourceFile();
        }
    }

    // http://rosecompiler.org/ROSE_HTML_Reference/namespaceSageBuilder.html#a9c9bb07f0244282e95da666ed3947940
    void SourceFileERTInserter::insertERTIntoForLoop(SgForStatement *for_stmt)
    {
        SageInterface::attachComment(for_stmt, "================ APERT ================");
        // Create a std::vector<ERT::RAW_TASK>
        SageInterface::addTextForUnparser(for_stmt, "{\n", AstUnparseAttribute::RelativePositionType::e_before);
        SageInterface::addTextForUnparser(for_stmt, "std::vector<ERT::RAW_TASK> " + this->ert_tasks_name_ + ";\n", AstUnparseAttribute::RelativePositionType::e_before);
        // Execute all tasks
        SageInterface::addTextForUnparser(for_stmt, "\n" + this->ert_pool_name_ + ".execute(std::move(" + this->ert_tasks_name_ + "));", AstUnparseAttribute::RelativePositionType::e_after);
        SageInterface::addTextForUnparser(for_stmt, "\n}", AstUnparseAttribute::RelativePositionType::e_after);

        SgStatement *body_stmt = SageInterface::getLoopBody(for_stmt);
        // Capture the loop body into a lambda task
        SageInterface::addTextForUnparser(body_stmt, "{\n", AstUnparseAttribute::RelativePositionType::e_before);
        SageInterface::addTextForUnparser(body_stmt, "auto " + this->ert_task_name_ + " = [=]()\n", AstUnparseAttribute::RelativePositionType::e_before);
        SageInterface::addTextForUnparser(body_stmt, ";", AstUnparseAttribute::RelativePositionType::e_after);
        // Insert the lambda task into the tasks list
        SageInterface::addTextForUnparser(body_stmt, "\n" + this->ert_tasks_name_ + ".emplace_back(std::move(" + this->ert_task_name_ + "));", AstUnparseAttribute::RelativePositionType::e_after);
        SageInterface::addTextForUnparser(body_stmt, "\n}", AstUnparseAttribute::RelativePositionType::e_after);

        this->is_ert_used_ = true;
        // if (Config::get().enable_debug)
        // {
        //     print_ast(for_stmt);
        // }
    }

    void SourceFileERTInserter::insertERTIntoFunction(SgFunctionDefinition *defn)
    {
        SgBasicBlock *body = defn->get_body();
        // Declare and initialize the pool
        SageInterface::addTextForUnparser(body, "{\n", AstUnparseAttribute::RelativePositionType::e_before);
        SageInterface::addTextForUnparser(body, this->ert_pool_type_ + " " + this->ert_pool_name_ + "(4);\n", AstUnparseAttribute::RelativePositionType::e_before);
        SageInterface::addTextForUnparser(body, this->ert_pool_name_ + ".start();\n", AstUnparseAttribute::RelativePositionType::e_before);
        SageInterface::addTextForUnparser(body, "\n}", AstUnparseAttribute::RelativePositionType::e_after);

        this->is_ert_used_ = true;
        // if (Config::get().enable_debug)
        // {
        //     print_ast(defn);
        // }
    }

    void SourceFileERTInserter::insertERTHeaderIntoSourceFile()
    {
        SageInterface::insertHeader(this->sfile_, this->ert_pool_type_include_, false, true);
    }
}
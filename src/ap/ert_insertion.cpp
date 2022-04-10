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

    // http://rosecompiler.org/ROSE_HTML_Reference/namespaceSageBuilder.html#a9c9bb07f0244282e95da666ed3947940
    void insertERTIntoForLoop(SgForStatement *for_stmt)
    {
        SageInterface::attachComment(for_stmt, "================ APERT ================");
        if (Config::get().enable_debug)
        {
            print_ast(for_stmt);
        }
    }
}
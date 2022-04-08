/*
 * Automatic Parallelization using OpenMP
 *
 * Input: sequential C/C++ code
 * Output: parallelized C/C++ code using OpenMP
 *
 * Algorithm:
 *   Read in semantics specification (formerly array abstraction) files
 *   Collect all loops with canonical forms
 *     x. Conduct loop normalization
 *     x. Call dependence analysis from Qing's loop transformations
 *     x. Conduct liveness analysis and variable classification
 *     x. Judge if the loop is parallelizable
 *     x. Attach OmpAttribute if it is
 *     x. Insert OpenMP pragma accordingly
 *
 * By Chunhua Liao
 * Nov 3, 2008
 *
 * autoParallelization Project:
 * https://en.wikibooks.org/wiki/ROSE_Compiler_Framework/autoPar#Alternative:_using_the_virtual_machine_image
 *
 * autoPar.cc from rose/projects/autoParallelization
 * Originally by Chunhua Liao
 *
 *
 * -----------------------------------------------------------
 * Refactored and modified by Lichen Liu
 *
 */
#include "auto_par_driver.h"
#include "auto_par_lib.h"

#include <iostream>

using namespace AutoParallelization;
using namespace SageInterface;

namespace
{
    void findCandidateFunctionDefinitions(SgProject *project, std::vector<SgFunctionDefinition *> &candidateFuncDefs)
    {
        ROSE_ASSERT(project != nullptr);
        // For each source file in the project
        const std::vector<SgFile *> &ptr_list = project->get_fileList();
        for (SgFile *sageFile : ptr_list)
        {
            SgSourceFile *sfile = isSgSourceFile(sageFile);
            ROSE_ASSERT(sfile);

            if (Config::get().enable_debug)
                std::cout << "Processing each function within the files " << sfile->get_file_info()->get_filename() << std::endl;

            std::vector<SgFunctionDefinition *> defList = querySubTree<SgFunctionDefinition>(sfile, V_SgFunctionDefinition);

            // For each function body in the scope
            for (SgFunctionDefinition *defn : defList)
            {
                SgFunctionDeclaration *func = defn->get_declaration();
                ROSE_ASSERT(func != nullptr);

                if (Config::get().enable_debug)
                    std::cout << "\t considering function " << func->get_name() << " at " << func->get_file_info()->get_line() << std::endl;
                // ignore functions in system headers, Can keep them to test robustness
                if (defn->get_file_info()->get_filename() != sageFile->get_file_info()->get_filename())
                {
                    if (Config::get().enable_debug)
                        std::cout << "\t Skipped since the function's associated file name does not match current file being considered. Mostly from a header. " << std::endl;
                    continue;
                }
                candidateFuncDefs.push_back(defn);
            } // end for def list
        }     // end for file list
    }

    // normalize all loops within candidate function definitions
    void normalizeLoops(std::vector<SgFunctionDefinition *> candidateFuncDefs)
    {
        for (SgFunctionDefinition *funcDef : candidateFuncDefs)
        {
            ROSE_ASSERT(funcDef);
            // This has to happen before analyses are called.
            // For each loop
            std::vector<SgForStatement *> loops = querySubTree<SgForStatement>(funcDef, V_SgForStatement);

            if (Config::get().enable_debug)
                std::cout << "Normalize loops queried from memory pool ...." << std::endl;

            // normalize C99 style for (int i= x, ...) to C89 style: int i;  (i=x, ...)
            // Liao, 10/22/2009. Thank Jeff Keasler for spotting this bug
            for (SgForStatement *cur_loop : loops)
            {
                if (Config::get().enable_debug)
                    std::cout << "\t loop at:" << cur_loop->get_file_info()->get_line() << std::endl;
                // skip for (;;) , SgForStatement::get_test_expr() has a buggy assertion.
                SgStatement *test_stmt = cur_loop->get_test();
                if (test_stmt != nullptr &&
                    isSgNullStatement(test_stmt))
                {
                    if (Config::get().enable_debug)
                        std::cout << "\t skipped due to empty loop header like for (;;)" << std::endl;
                    continue;
                }

                // skip system header
                if (insideSystemHeader(cur_loop))
                {
                    if (Config::get().enable_debug)
                        std::cout << "\t skipped since the loop is inside a system header " << std::endl;
                    continue;
                }
                SageInterface::forLoopNormalization(cur_loop);
            } // end for all loops
        }     // end for all function defs
    }
}

namespace AutoParallelization
{
    void auto_parallize(SgProject *project, bool enable_debug)
    {
        ROSE_ASSERT(project != nullptr);

        {
            Config::get().enable_debug = enable_debug;
        }

        // create a block to avoid jump crosses initialization of candidateFuncDefs etc.
        {
            std::vector<SgFunctionDefinition *> candidateFuncDefs;
            findCandidateFunctionDefinitions(project, candidateFuncDefs);
            normalizeLoops(candidateFuncDefs);

            // Prepare liveness analysis etc.
            initialize_analysis(project, false);

            // This is a bit redundant with findCandidateFunctionDefinitions ()
            // But we do need the per file control to decide if omp.h is needed for each file
            //
            // For each source file in the project
            const std::vector<SgFile *> &ptr_list = project->get_fileList();
            for (SgFile *sageFile : ptr_list)
            {
                SgSourceFile *sfile = isSgSourceFile(sageFile);
                ROSE_ASSERT(sfile);
                SgGlobal *root = sfile->get_globalScope();

                std::vector<SgFunctionDefinition *> defList = querySubTree<SgFunctionDefinition>(sfile, V_SgFunctionDefinition);
                // flag to indicate if there is at least one loop is parallelized. Also means execution runtime headers are needed
                bool hasERT = false;

                // For each function body in the scope
                for (SgFunctionDefinition *defn : defList)
                {
                    //      std::cout<<"\t loop at:"<< cur_loop->get_file_info()->get_line() <<std::endl;

                    SgFunctionDeclaration *func = defn->get_declaration();
                    ROSE_ASSERT(func != nullptr);

                    // ignore functions in system headers, Can keep them to test robustness
                    if (defn->get_file_info()->get_filename() != sageFile->get_file_info()->get_filename())
                    {
                        continue;
                    }

                    SgBasicBlock *body = defn->get_body();
                    // For each loop
                    std::vector<SgForStatement *> loops = querySubTree<SgForStatement>(defn, V_SgForStatement);
                    if (loops.size() == 0)
                    {
                        if (Config::get().enable_debug)
                            std::cout << "\t skipped since no for loops are found in this function" << std::endl;
                        continue;
                    }

                    // X. Replace operators with their equivalent counterparts defined
                    // in "inline" annotations
                    AstInterfaceImpl faImpl_1(body);
                    CPPAstInterface fa_body(&faImpl_1);
                    OperatorInlineRewrite()(fa_body, AstNodePtrImpl(body));

                    // Pass annotations to arrayInterface and use them to collect
                    // alias info. function info etc.
                    ArrayAnnotation *annot = ArrayAnnotation::get_inst();
                    ArrayInterface array_interface(*annot);
                    // alias Collect
                    // value collect
                    array_interface.initialize(fa_body, AstNodePtrImpl(defn));
                    // valueCollect
                    array_interface.observe(fa_body);

                    // FR(06/07/2011): aliasinfo was not set which caused segfault
                    LoopTransformInterface::set_aliasInfo(&array_interface);

                    std::vector<SgForStatement *> parallelizable_loop_candidates;
                    for (SgForStatement *current_loop : loops)
                    {
                        if (Config::get().enable_debug)
                        {
                            std::cout << std::endl;
                            std::cout << "\t\t ------------------------------" << std::endl;
                            std::cout << "\t\t | Considering loop at " << current_loop->get_file_info()->get_line() << std::endl;
                            std::cout << "\t\t ------------------------------" << std::endl;
                        }
                        // X. Parallelize loop one by one
                        //  getLoopInvariant() will actually check if the loop has canonical forms
                        //  which can be handled by dependence analysis

                        // skip loops with unsupported language features
                        VariantT blackConstruct;
                        if (useUnsupportedLanguageFeatures(current_loop, &blackConstruct))
                        {
                            if (Config::get().enable_debug)
                                std::cout << "Skipping a loop at line:" << current_loop->get_file_info()->get_line() << " due to unsupported language construct " << blackConstruct << "..." << std::endl;
                            continue;
                        }

                        SgInitializedName *invarname = getLoopInvariant(current_loop);
                        if (invarname != nullptr)
                        {
                            bool ret = CanParallelizeOutermostLoop(current_loop, &array_interface, annot);
                            if (ret)
                            {
                                parallelizable_loop_candidates.emplace_back(current_loop);
                            }
                        }
                        else // cannot grab loop index from a non-conforming loop, skip parallelization
                        {
                            if (Config::get().enable_debug)
                                std::cout << "Skipping a non-canonical loop at line:" << current_loop->get_file_info()->get_line() << "..." << std::endl;
                        }
                    } // end for loops

                    if (!parallelizable_loop_candidates.empty())
                    {
                        hasERT = true;
                    }
                } // end for-loop for declarations

                // insert ERT-related files if needed
                if (hasERT)
                {
                    SageInterface::insertHeader("omp.h", PreprocessingInfo::after, true, root);
                    std::cout << std::endl;
                    std::cout << "=====================================================" << std::endl;
                    std::cout << "Successfully found parallelizable loops and added Execution Runtime for parallelization!" << std::endl;
                }
            } // end for-loop of files

            // undo loop normalization
            for (auto [for_loop, _] : trans_records.forLoopInitNormalizationTable)
            {
                unnormalizeForLoopInitDeclaration(for_loop);
            }
            // Qing's loop normalization is not robust enough to pass all tests
            // AstTests::runAllTests(project);

            // clean up resources for analyses
            release_analysis();
        }
    }
}

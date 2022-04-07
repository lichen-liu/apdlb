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
 */
#include "rose.h"
// all kinds of analyses needed
#include "autoParSupport.h"
#include <string>
#include <Rose/CommandLine.h>
#include <Sawyer/CommandLine.h>
static const char *purpose = "This tool automatically inserts OpenMP directives into sequential codes.";
static const char *description =
    "This tool is an implementation of automatic parallelization using OpenMP. "
    "It can automatically insert OpenMP directives into input serial C/C++ codes. ";

using namespace std;
using namespace AutoParallelization;
using namespace SageInterface;

void findCandidateFunctionDefinitions(SgProject *project, std::vector<SgFunctionDefinition *> &candidateFuncDefs)
{
    ROSE_ASSERT(project != NULL);
    // For each source file in the project
    SgFilePtrList &ptr_list = project->get_fileList();
    for (SgFilePtrList::iterator iter = ptr_list.begin(); iter != ptr_list.end();
         iter++)
    {
        SgFile *sageFile = (*iter);
        SgSourceFile *sfile = isSgSourceFile(sageFile);
        ROSE_ASSERT(sfile);
        //    SgGlobal *root = sfile->get_globalScope();

        if (Config::get().enable_debug)
            cout << "Processing each function within the files " << sfile->get_file_info()->get_filename() << endl;
        //      cout<<"\t loop at:"<< cur_loop->get_file_info()->get_line() <<endl;

        // This is wrong, many functions in question are not top level declarations!!
        // SgDeclarationStatementPtrList& declList = root->get_declarations ();
        // VariantVector vv;
        Rose_STL_Container<SgNode *> defList = NodeQuery::querySubTree(sfile, V_SgFunctionDefinition);
        //    bool hasOpenMP= false; // flag to indicate if omp.h is needed in this file

        // For each function body in the scope
        // for (SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p)
        for (Rose_STL_Container<SgNode *>::iterator p = defList.begin(); p != defList.end(); ++p)
        {
            SgFunctionDefinition *defn = isSgFunctionDefinition(*p);
            ROSE_ASSERT(defn != NULL);

            SgFunctionDeclaration *func = defn->get_declaration();
            ROSE_ASSERT(func != NULL);

            if (Config::get().enable_debug)
                cout << "\t considering function " << func->get_name() << " at " << func->get_file_info()->get_line() << endl;
            // ignore functions in system headers, Can keep them to test robustness
            if (defn->get_file_info()->get_filename() != sageFile->get_file_info()->get_filename())
            {
                if (Config::get().enable_debug)
                    cout << "\t Skipped since the function's associated file name does not match current file being considered. Mostly from a header. " << endl;
                continue;
            }
            candidateFuncDefs.push_back(defn);
        } // end for def list
    }     // end for file list
}

// normalize all loops within candidate function definitions
void normalizeLoops(std::vector<SgFunctionDefinition *> candidateFuncDefs)
{
    for (std::vector<SgFunctionDefinition *>::iterator iter = candidateFuncDefs.begin(); iter != candidateFuncDefs.end(); iter++)
    {
        SgFunctionDefinition *funcDef = *iter;
        ROSE_ASSERT(funcDef);
        // This has to happen before analyses are called.
        // For each loop
        VariantVector vv(V_SgForStatement);
        Rose_STL_Container<SgNode *> loops = NodeQuery::querySubTree(funcDef, vv);

        if (Config::get().enable_debug)
            cout << "Normalize loops queried from memory pool ...." << endl;

        // normalize C99 style for (int i= x, ...) to C89 style: int i;  (i=x, ...)
        // Liao, 10/22/2009. Thank Jeff Keasler for spotting this bug
        for (Rose_STL_Container<SgNode *>::iterator iter = loops.begin();
             iter != loops.end(); iter++)
        {
            SgForStatement *cur_loop = isSgForStatement(*iter);
            ROSE_ASSERT(cur_loop);

            if (Config::get().enable_debug)
                cout << "\t loop at:" << cur_loop->get_file_info()->get_line() << endl;
            // skip for (;;) , SgForStatement::get_test_expr() has a buggy assertion.
            SgStatement *test_stmt = cur_loop->get_test();
            if (test_stmt != NULL &&
                isSgNullStatement(test_stmt))
            {
                if (Config::get().enable_debug)
                    cout << "\t skipped due to empty loop header like for (;;)" << endl;
                continue;
            }

            // skip system header
            if (insideSystemHeader(cur_loop))
            {
                if (Config::get().enable_debug)
                    cout << "\t skipped since the loop is inside a system header " << endl;
                continue;
            }
            SageInterface::forLoopNormalization(cur_loop);
        } // end for all loops
    }     // end for all function defs
}

//! Initialize the switch group and its switches.
[[maybe_unused]] static Sawyer::CommandLine::SwitchGroup commandLineSwitches()
{
    using namespace Sawyer::CommandLine;

    SwitchGroup switches("autoPar's switches");
    switches.doc("These switches control the autoPar tool. ");
    switches.name("rose:autopar");

    switches.insert(Switch("no_aliasing")
                        .intrinsicValue(true, Config::get().no_aliasing)
                        .doc("Assuming no pointer aliasing exists."));

    switches.insert(Switch("unique_indirect_index")
                        .intrinsicValue(true, Config::get().b_unique_indirect_index)
                        .doc("Assuming all arrays used as indirect indices have unique elements (no overlapping)"));

    switches.insert(Switch("annot")
                        .argument("string", anyParser(Config::get().annot_filenames))
                        //      .shortPrefix("-") // this option allows short prefix
                        .whichValue(SAVE_ALL) // if switch appears more than once, save all values not just last
                        .doc("Specify semantics annotation file for standard or user-defined abstractions"));

    return switches;
}

// New version of command line processing using Sawyer library
[[maybe_unused]] static std::vector<std::string> commandline_processing(std::vector<std::string> &argvList)
{
    using namespace Sawyer::CommandLine;
    Parser p = Rose::CommandLine::createEmptyParserStage(purpose, description);
    p.doc("Synopsis", "@prop{programName} @v{switches} @v{files}...");
    p.longPrefix("-");

    // initialize generic Sawyer switches: assertion, logging, threads, etc.
    p.with(Rose::CommandLine::genericSwitches());

    // initialize this tool's switches
    p.with(commandLineSwitches());

    // --rose:help for more ROSE switches
    SwitchGroup tool("ROSE builtin switches");
    bool showRoseHelp = false;
    tool.insert(Switch("rose:help")
                    .longPrefix("-")
                    .intrinsicValue(true, showRoseHelp)
                    .doc("Show the old-style ROSE help."));
    p.with(tool);

    std::vector<std::string> remainingArgs = p.parse(argvList).apply().unparsedArgs(true);

    // add back -annot file TODO: how about multiple appearances?
    for (size_t i = 0; i < Config::get().annot_filenames.size(); i++)
    {
        remainingArgs.push_back("-annot");
        remainingArgs.push_back(Config::get().annot_filenames[i]);
    }

    // AFTER parse the command-line, you can do this:
    if (showRoseHelp)
        SgFile::usage(0);

    // work with the parser of the ArrayAbstraction module
    // Read in annotation files after -annot
    CmdOptions::GetInstance()->SetOptions(remainingArgs);
    ArrayAnnotation *annot = ArrayAnnotation::get_inst();
    annot->register_annot();
    ReadAnnotation::get_inst()->read();

    // Strip off custom options and their values to enable backend compiler
    CommandlineProcessing::removeArgsWithParameters(remainingArgs, "-annot");

    return remainingArgs;
}

int main(int argc, char *argv[])
{
    ROSE_INITIALIZE;

    // vector<string> argvList(argv, argv + argc);
    // argvList = commandline_processing(argvList);
    SgProject *project = frontend(argc, argv);
    ROSE_ASSERT(project != NULL);

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
        SgFilePtrList &ptr_list = project->get_fileList();
        for (SgFilePtrList::iterator iter = ptr_list.begin(); iter != ptr_list.end();
             iter++)
        {
            SgFile *sageFile = (*iter);
            SgSourceFile *sfile = isSgSourceFile(sageFile);
            ROSE_ASSERT(sfile);
            SgGlobal *root = sfile->get_globalScope();

            Rose_STL_Container<SgNode *> defList = NodeQuery::querySubTree(sfile, V_SgFunctionDefinition);
            // flag to indicate if there is at least one loop is parallelized. Also means execution runtime headers are needed
            bool hasERT = false;

            // For each function body in the scope
            // for (SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p)
            for (Rose_STL_Container<SgNode *>::iterator p = defList.begin(); p != defList.end(); ++p)
            {

                //      cout<<"\t loop at:"<< cur_loop->get_file_info()->get_line() <<endl;

                SgFunctionDefinition *defn = isSgFunctionDefinition(*p);
                ROSE_ASSERT(defn != NULL);

                SgFunctionDeclaration *func = defn->get_declaration();
                ROSE_ASSERT(func != NULL);

                // ignore functions in system headers, Can keep them to test robustness
                if (defn->get_file_info()->get_filename() != sageFile->get_file_info()->get_filename())
                {
                    continue;
                }

                SgBasicBlock *body = defn->get_body();
                // For each loop
                Rose_STL_Container<SgNode *> loops = NodeQuery::querySubTree(defn, V_SgForStatement);
                if (loops.size() == 0)
                {
                    if (Config::get().enable_debug)
                        cout << "\t skipped since no for loops are found in this function" << endl;
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

                for (Rose_STL_Container<SgNode *>::iterator iter = loops.begin();
                     iter != loops.end(); iter++)
                {
                    SgNode *current_loop = *iter;

                    if (Config::get().enable_debug)
                    {
                        SgForStatement *fl = isSgForStatement(current_loop);
                        cout << "\t\t Considering loop at " << fl->get_file_info()->get_line() << endl;
                    }
                    // X. Parallelize loop one by one
                    //  getLoopInvariant() will actually check if the loop has canonical forms
                    //  which can be handled by dependence analysis

                    // skip loops with unsupported language features
                    VariantT blackConstruct;
                    if (useUnsupportedLanguageFeatures(current_loop, &blackConstruct))
                    {
                        if (Config::get().enable_debug)
                            cout << "Skipping a loop at line:" << current_loop->get_file_info()->get_line() << " due to unsupported language construct " << blackConstruct << "..." << endl;
                        continue;
                    }

                    SgInitializedName *invarname = getLoopInvariant(current_loop);
                    if (invarname != NULL)
                    {
                        bool ret = ParallelizeOutermostLoop(current_loop, &array_interface, annot);
                        if (ret) // if at least one loop is parallelized, we set hasERT to be true for the entire file.
                            hasERT = true;
                    }
                    else // cannot grab loop index from a non-conforming loop, skip parallelization
                    {
                        if (Config::get().enable_debug)
                            cout << "Skipping a non-canonical loop at line:" << current_loop->get_file_info()->get_line() << "..." << endl;
                        // We should not reset it to false. The last loop may not be parallelizable. But a previous one may be.
                        // hasERT = false;
                    }
                } // end for loops
            }     // end for-loop for declarations

            // insert ERT-related files if needed
            if (hasERT)
            {
                SageInterface::insertHeader("omp.h", PreprocessingInfo::after, true, root);
                cout << endl;
                cout << "Successfully found parallelizable loops and added Execution Runtime for parallelization!" << endl;
            }
        } // end for-loop of files

        // undo loop normalization
        std::map<SgForStatement *, bool>::iterator iter = trans_records.forLoopInitNormalizationTable.begin();
        for (; iter != trans_records.forLoopInitNormalizationTable.end(); iter++)
        {
            SgForStatement *for_loop = (*iter).first;
            unnormalizeForLoopInitDeclaration(for_loop);
        }
        // Qing's loop normalization is not robust enough to pass all tests
        // AstTests::runAllTests(project);

        // clean up resources for analyses
        release_analysis();
    }

    // Report errors
    int status = backend(project);
    // we always write to log files by default now
    return status;
}

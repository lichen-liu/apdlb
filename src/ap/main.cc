/*
 * A boiler plate starter file for using ROSE
 *
 * Input: sequential C/C++ code
 * Output: same C/C++ code
 *
 * ROSE API: http://rosecompiler.org/ROSE_HTML_Reference/
 * Tutorial: /u/course/ece1754/rose-0.11.63.0/projects/autoParallelization/
 * https://github.com/rose-compiler/rose/wiki/Tool-Developer-Tutorial
 *
 */

#include <iostream>

#include "utils.h"
#include "archived.h"
#include "driver.h"
#include "rose.h"

int main(int argc, char *argv[])
{
    constexpr bool debug = true;

    ROSE_INITIALIZE;

    // Build a project
    SgProject *project = frontend(argc, argv);

    AutoParallelization::auto_parallize(project, debug);

    return backend(project);

#if 0
    ROSE_ASSERT(project);

    // For each source file in the project
    SgFilePtrList &ptr_list = project->get_fileList();

    for (SgFilePtrList::iterator iter = ptr_list.begin(); iter != ptr_list.end(); iter++)
    {
        SgFile *sageFile = *iter;
        SgSourceFile *sfile = isSgSourceFile(sageFile);
        ROSE_ASSERT(sfile);
        SgGlobal *root = sfile->get_globalScope();
        SgDeclarationStatementPtrList &declList = root->get_declarations();

        std::cout << "Found a file" << std::endl;
        std::cout << "  " << sfile->get_file_info()->get_filename() << std::endl;

        // For each function body in the scope
        for (SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p)
        {
            SgFunctionDeclaration *func = isSgFunctionDeclaration(*p);
            if (func == nullptr)
                continue;
            SgFunctionDefinition *defn = func->get_definition();
            if (defn == nullptr)
                continue;
            // Ignore functions in system headers, Can keep them to test robustness
            if (defn->get_file_info()->get_filename() != sageFile->get_file_info()->get_filename())
                continue;

            AP::process_function_body(defn, debug);
        } // end for-loop for declarations
    }     // end for-loop for files

    std::cout << "Done ..." << std::endl;
#endif
}
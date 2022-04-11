/*
 * ROSE API: http://rosecompiler.org/ROSE_HTML_Reference/
 * Tutorial: /u/course/ece1754/rose-0.11.63.0/projects/autoParallelization/
 * https://github.com/rose-compiler/rose/wiki/Tool-Developer-Tutorial
 */

#include <iostream>
#include <filesystem>
#include <vector>

#include "driver.h"
#include "rose.h"

int main(int argc, char *argv[])
{
    ROSE_INITIALIZE;

    // Prepare args
    std::vector<std::string> args(argv, argv + argc);
    std::vector<std::string> processed_args;
    int target_nthreads = 8;
    bool enable_debug = false;
    AP::ERT_TYPE ert_type = AP::ERT_TYPE::DEFAULT;

    // Parse args
    for (const auto &arg : args)
    {
        if (auto pos = arg.find("-j"); pos != std::string::npos)
        {
            std::string target_nthreads_str = arg.substr(pos + 2);
            target_nthreads = std::stoi(target_nthreads_str);
        }
        else if (auto pos = arg.find("-e"); pos != std::string::npos)
        {
            std::string ert_type_str = arg.substr(pos + 2);
            int ert_type_int = std::stoi(ert_type_str);
            ert_type = static_cast<AP::ERT_TYPE>(ert_type_int);
        }
        else if (arg == "-d")
        {
            enable_debug = true;
        }
        else
        {
            processed_args.emplace_back(arg);
        }
    }

    // Post-process args
    if (target_nthreads <= 0)
    {
        target_nthreads = -1;
    }
    std::cout << std::endl;
    std::cout << "Args:" << std::endl;
    std::cout << "target_nthreads=" << target_nthreads << std::endl;
    std::cout << "ert_type=" << static_cast<int>(ert_type) << std::endl;
    std::cout << "enable_debug=" << enable_debug << std::endl;
    std::cout << std::endl;

    // Build a project
    SgProject *project = frontend(processed_args);

    // Auto parallelization
    AutoParallelization::auto_parallize(project, target_nthreads, ert_type, enable_debug);

    // Generate code
    const std::string gen_code_dir = std::filesystem::current_path() / "apert_gen";
    const std::vector<SgFile *> &ptr_list = project->get_fileList();
    for (SgFile *file : ptr_list)
    {
        SageInterface::moveToSubdirectory(gen_code_dir, file);
    }
    project->unparse();
    std::cout << std::endl;
    std::cout << "Code generated at: " << gen_code_dir << std::endl;
    std::cout << "Done Sucessfully!" << std::endl;

    // Clean up the project
    delete project;
    project = nullptr;

    return 0;
}
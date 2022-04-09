#pragma once

#include <vector>
#include <string>

namespace AP
{
    struct Config
    {
        bool enable_debug = true;            // maximum debugging output to the screen
        bool no_aliasing = false;            // assuming aliasing or not
        bool b_unique_indirect_index = true; // assume all arrays used as indirect indices has unique elements(no overlapping)
        std::vector<std::string> annot_filenames;

        static Config &get()
        {
            static Config conf;
            return conf;
        }
    };
}
#pragma once
#include <thread>
#include <sstream>

namespace TP
{
    inline const char *bool_to_cstr(bool val)
    {
        return val ? "true" : "false";
    }

    inline std::string to_string(std::thread::id id)
    {
        std::stringstream ss;
        ss << id;
        return ss.str();
    }
}
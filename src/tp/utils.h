#pragma once
#include <thread>

namespace TP
{
    const char *bool_to_cstr(bool val);

    std::string to_string(std::thread::id id);
}
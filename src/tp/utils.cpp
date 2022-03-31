#include "utils.h"
#include <sstream>

namespace TP
{
    const char *bool_to_cstr(bool val)
    {
        return val ? "true" : "false";
    }

    std::string to_string(std::thread::id id)
    {
        std::stringstream ss;
        ss << id;
        return ss.str();
    }
}
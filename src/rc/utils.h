#pragma once

#include <string>

#include "rose.h"

namespace RC
{
    std::string to_string(SgNode *node);

    std::string get_indent(int indent);

    void print_ast(SgNode *root);

    void serialize(SgNode *node, const std::string &prefix, bool hasRemaining, std::ostringstream &out);
    void serialize(const SgTemplateArgumentPtrList &plist, const std::string &prefix, bool hasRemaining, std::ostringstream &out);
    void serialize(SgNode *node, const std::string &prefix, bool hasRemaining, std::ostringstream &out);
}
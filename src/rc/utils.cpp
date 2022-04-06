#include "utils.h"

#include <sstream>

namespace rc
{
    std::string to_string(SgNode *node)
    {
        std::ostringstream ss;
        serialize(node, "", false, ss);
        return ss.str();
    }

    std::string get_indent(int indent)
    {
        return std::string(2 * indent, ' ');
    }

    void print_ast(SgNode *root)
    {
        std::cout << std::endl;
        std::cout << "====================== DEBUG ======================" << std::endl;
        SageInterface::printAST(root);
        std::cout << "====================== DEBUG ======================" << std::endl;
        std::cout << std::endl;
    }

    // From sageInterface.C, modified to print only the current node
    // A special node in the AST text dump
    void serialize(const SgTemplateArgumentPtrList &plist, const std::string &prefix, bool hasRemaining, std::ostringstream &out)
    {
        out << prefix;
        out << (hasRemaining ? "|---" : "|___");

        // print address
        out << "@" << &plist << " "
            << "SgTemplateArgumentPtrList ";

        out << std::endl;
        for (size_t i = 0; i < plist.size(); i++)
        {
            bool n_hasRemaining = false;
            if (i + 1 < plist.size())
                n_hasRemaining = true;
            std::string suffix = hasRemaining ? "|   " : "    ";
            std::string n_prefix = prefix + suffix;
            serialize(plist[i], n_prefix, n_hasRemaining, out);
        }
    }

    // From sageInterface.C, modified to print only the current node
    void serialize(SgNode *node, const std::string &prefix, bool hasRemaining, std::ostringstream &out)
    {
        // there may be NULL children!!
        // if (!node) return;

        out << prefix;
        // out << (hasRemaining ? "|---" : "|___");
        if (!node)
        {
            out << " NULL " << std::endl;
            return;
        }

        // print address
        out << "@" << node << " " << node->class_name() << " ";

        // optionally file info
        if (SgLocatedNode *lnode = isSgLocatedNode(node))
        {
            out << Rose::StringUtility::stripPathFromFileName(lnode->get_file_info()->get_filename()) << " " << lnode->get_file_info()->get_line() << ":" << lnode->get_file_info()->get_col();
        }

        // optionally  qualified name
        if (SgFunctionDeclaration *f = isSgFunctionDeclaration(node))
            out << " " << f->get_qualified_name();

        if (SgClassDeclaration *f = isSgClassDeclaration(node))
            out << " " << f->get_qualified_name();

        if (SgTypedefDeclaration *f = isSgTypedefDeclaration(node))
            out << " " << f->get_qualified_name();

        if (SgAdaPackageSpecDecl *f = isSgAdaPackageSpecDecl(node))
            out << " " << f->get_qualified_name();

        if (SgInitializedName *v = isSgInitializedName(node))
        {
            out << " " << v->get_qualified_name();
            out << " type@" << v->get_type();
            out << " initializer@" << v->get_initializer();
            //    type_set.insert (v->get_type());
        }

        // associated class, function and variable declarations
        if (SgTemplateInstantiationDecl *f = isSgTemplateInstantiationDecl(node))
            out << " template class decl@" << f->get_templateDeclaration();

        if (SgMemberFunctionDeclaration *f = isSgMemberFunctionDeclaration(node))
            out << " assoc. class decl@" << f->get_associatedClassDeclaration();

        if (SgConstructorInitializer *ctor = isSgConstructorInitializer(node))
        {
            out << " member function decl@" << ctor->get_declaration();
        }

        if (SgIntVal *v = isSgIntVal(node))
            out << " value=" << v->get_value() << " valueString=" << v->get_valueString();

        if (SgVarRefExp *var_ref = isSgVarRefExp(node))
            out << " init name@" << var_ref->get_symbol()->get_declaration() << " symbol name=" << var_ref->get_symbol()->get_name();

        if (SgMemberFunctionRefExp *func_ref = isSgMemberFunctionRefExp(node))
            out << " member func decl@" << func_ref->get_symbol_i()->get_declaration();

        if (SgTemplateInstantiationMemberFunctionDecl *cnode = isSgTemplateInstantiationMemberFunctionDecl(node))
            out << " template member func decl@" << cnode->get_templateDeclaration();

        if (SgFunctionRefExp *func_ref = isSgFunctionRefExp(node))
        {
            SgFunctionSymbol *sym = func_ref->get_symbol_i();
            out << " func decl@" << sym->get_declaration() << " func sym name=" << sym->get_name();
        }

        // base type of several types of nodes:
        if (SgTypedefDeclaration *v = isSgTypedefDeclaration(node))
        {
            out << " base_type@" << v->get_base_type();
            //    type_set.insert (v->get_base_type());
        }

        if (SgArrayType *v = isSgArrayType(node))
            out << " base_type@" << v->get_base_type();

        if (SgTypeExpression *v = isSgTypeExpression(node))
            out << " type@" << v->get_type();

        if (SgAdaAttributeExp *v = isSgAdaAttributeExp(node))
            out << " attribute@" << v->get_attribute();

        if (SgDeclarationStatement *v = isSgDeclarationStatement(node))
        {
            out << " first nondefining decl@" << v->get_firstNondefiningDeclaration();
            out << " defining decl@" << v->get_definingDeclaration();
        }

        // out << std::endl;

        std::vector<SgNode *> children = node->get_traversalSuccessorContainer();
        int total_count = children.size();
        int current_index = 0;

        // some Sg??PtrList are not AST nodes, not part of children , we need to handle them separatedly
        // we sum all children into single total_count to tell if there is remaining children.
        if (isSgTemplateInstantiationDecl(node))
            total_count += 1; // sn->get_templateArguments().size();

        // handling SgTemplateArgumentPtrList first
        if (SgTemplateInstantiationDecl *sn = isSgTemplateInstantiationDecl(node))
        {
            const SgTemplateArgumentPtrList &plist = sn->get_templateArguments();
            bool n_hasRemaining = false;
            if (current_index + 1 < total_count)
                n_hasRemaining = true;
            current_index++;

            std::string suffix = hasRemaining ? "|   " : "    ";
            std::string n_prefix = prefix + suffix;
            out << std::endl;
            serialize(plist, n_prefix, n_hasRemaining, out);
        }

        // finish sucessors
        // for (size_t i = 0; i < children.size(); i++)
        // {
        //     bool n_hasRemaining = false;
        //     if (current_index + 1 < total_count)
        //         n_hasRemaining = true;
        //     current_index++;

        //     string suffix = hasRemaining ? "|   " : "    ";
        //     string n_prefix = prefix + suffix;
        //     serialize(children[i], n_prefix, n_hasRemaining, out);
        // }
    }
}
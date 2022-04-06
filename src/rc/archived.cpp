#include "archived.h"

#include <unordered_set>
#include "utils.h"

namespace RC
{
    // data dependence testing pair
    struct RawDDTP
    {
        SgPntrArrRefExp *w_array_ref;
        SgPntrArrRefExp *target_array_ref;
        std::vector<SgForStatement *> outer_for_stmts; // from inner to outer
    };

    struct DDTP
    {
        SgPntrArrRefExp *w_array_ref;
        SgPntrArrRefExp *target_array_ref;
        std::vector<SgInitializedName *> common_induction_vars; // from outer to inner
    };

    std::ostream &operator<<(std::ostream &os, const DDTP &ddtp)
    {
        os << ddtp.w_array_ref->unparseToString() << " : " << ddtp.target_array_ref->unparseToString() << " : ";
        for (SgInitializedName *var : ddtp.common_induction_vars)
        {
            os << var->get_name().getString() << ", ";
        }
        return os;
    }

    struct DDTPCollection
    {
        std::vector<DDTP> write_write_s;
        std::vector<DDTP> write_read_s;
    };

    std::ostream &operator<<(std::ostream &os, const DDTPCollection &ddtpc)
    {
        os << std::endl;
        os << "write ref : write ref : common surrounding loop indices" << std::endl;
        for (const auto &ww : ddtpc.write_write_s)
        {
            os << ww << std::endl;
        }

        os << std::endl;
        os << "write ref : read ref : common surrounding loop indices" << std::endl;
        for (const auto &wr : ddtpc.write_read_s)
        {
            os << wr << std::endl;
        }

        return os;
    }

    SgInitializedName *get_array_name_from_ref(SgPntrArrRefExp *array_ref, bool debug = false, int indent = 0)
    {
        SgExpression *array_name_exp = nullptr;
        std::unique_ptr<std::vector<SgExpression *>> array_ref_subscripts = std::make_unique<std::vector<SgExpression *>>();
        std::vector<SgExpression *> *array_ref_subscripts_p = array_ref_subscripts.get();
        // What a stupid API??? You only need to modify a vector but why a double pointer?
        SageInterface::isArrayReference(array_ref, &array_name_exp, &array_ref_subscripts_p);

        SgInitializedName *array_name = SageInterface::convertRefToInitializedName(array_name_exp);
        ROSE_ASSERT(array_name);

        if (debug)
        {
            const std::string indent_str = RC::get_indent(indent);
            const std::string inner_indent_str = RC::get_indent(1 + indent);
            std::cout << indent_str << "array_name: " << RC::to_string(array_name) << std::endl;
            std::cout << indent_str << "array: " << RC::to_string(array_name_exp) << std::endl;
            std::cout << indent_str << "subscript:" << std::endl;
            for (SgExpression *s : *array_ref_subscripts)
            {
                std::cout << inner_indent_str << RC::to_string(s) << std::endl;
            }
        }
        return array_name;
    }

    void for_stmt_ancestor_visitor(SgForStatement *from, SgScopeStatement *end, std::function<bool(SgForStatement *)> worker, bool debug)
    {
        ROSE_ASSERT(from);
        ROSE_ASSERT(end);
        if (debug)
        {
            std::cout << "ancestor traversal: " << RC::to_string(from) << std::endl;
        }
        SgStatement *cur_node = from;
        while (cur_node != end)
        {
            if (debug)
            {
                std::cout << "  " << RC::to_string(cur_node) << std::endl;
            }

            if (SgScopeStatement *enclosing_loop_stmt = SageInterface::findEnclosingLoop(cur_node))
            {
                if (SgForStatement *enclosing_for_stmt = isSgForStatement(enclosing_loop_stmt))
                {
                    if (!worker(enclosing_for_stmt))
                    {
                        return;
                    }
                }
            }

            ROSE_ASSERT(SageInterface::getEnclosingScope(cur_node) != cur_node);
            cur_node = SageInterface::getEnclosingScope(cur_node);
        }
    }

    std::vector<SgForStatement *> get_outer_for_stmts(SgForStatement *from, SgScopeStatement *end, bool debug = false)
    {
        std::vector<SgForStatement *> res;
        if (from)
        {
            // Do a tree traversal upwards to find all ancestors, including itself
            for_stmt_ancestor_visitor(
                from, end, [&res](SgForStatement *enclosing_for_stmt)
                {
                    res.push_back(enclosing_for_stmt);
                    return true; },
                debug);

            // There may be duplicates
            res.erase(std::unique(res.begin(), res.end()), res.end());
        }

        return res;
    }

    SgForStatement *find_common_ancestor_for_stmt(SgForStatement *n1, SgForStatement *n2, SgScopeStatement *end, bool debug = false)
    {
        if (n1 == n2)
            return n1;
        if (SageInterface::isAncestor(n1, n2))
            return n1;
        if (SageInterface::isAncestor(n2, n1))
            return n2;

        std::unordered_set<SgForStatement *> n1_ancestors;
        // Do a tree traversal upwards to find all n1 ancestors, including itself
        for_stmt_ancestor_visitor(
            n1, end, [&n1_ancestors](SgForStatement *enclosing_for_stmt)
            {
                n1_ancestors.insert(enclosing_for_stmt);
                return true; },
            debug);

        // Do a tree traversal upwards to find all n2 ancestors
        SgForStatement *common_ancestor = nullptr;
        for_stmt_ancestor_visitor(
            n2, end, [&n1_ancestors, &common_ancestor](SgForStatement *enclosing_for_stmt)
            {
                if (n1_ancestors.count(enclosing_for_stmt))
                {
                    common_ancestor = enclosing_for_stmt;
                    // early exit as soon as we hit an n1 ancestor
                    return false;
                }
                return true; },
            debug);

        return common_ancestor;
    }

    SgForStatement *get_enclosing_for_stmt(SgPntrArrRefExp *array_ref)
    {
        SgStatement *enclosing_stmt = SageInterface::getEnclosingStatement(array_ref);
        SgScopeStatement *enclosing_loop_stmt = SageInterface::findEnclosingLoop(enclosing_stmt);
        if (enclosing_loop_stmt)
        {
            return isSgForStatement(enclosing_loop_stmt);
        }

        return nullptr;
    }

    SgInitializedName *is_loop_analyzable(SgNode *for_loop_node, bool debug = false, bool verbose = false)
    {
        // Determine the “analyzable” loop
        // An analyzable loop is defined as a for loop that has an induction variable (call it i) with:
        // a. One or more initialization statements,
        // b. A single test expression that uses <, <=, > or >= operations,
        // c. A single increment expression in the form i=i+c, i=i-c, ++i, --i, i++
        //    or ++i, where c is a compile time constant, and
        // d. The value of i is not changed in the loop

        if (debug)
        {
            std::cout << RC::to_string(for_loop_node) << std::endl;
            std::cout << for_loop_node->unparseToString() << std::endl;
        }

        SgInitializedName *ivar = nullptr;
        SgExpression *lb = nullptr;
        SgExpression *ub = nullptr;
        SgExpression *step = nullptr;
        SgStatement *body = nullptr;

        // A canonical form is defined as:
        // a. one initialization statement
        // b. a test expression
        // c. an increment expression
        // d. loop index variable should be of an integer type

        // TODO: handwrite one that supports multiple initializations
        bool is_canonical = SageInterface::isCanonicalForLoop(for_loop_node, &ivar, &lb, &ub, &step, &body);

        if (debug)
        {
            const std::string indent = RC::get_indent(1);
            std::cout << indent << "var:" << std::endl;
            SageInterface::printAST(ivar);

            std::cout << indent << "lb:" << std::endl;
            SageInterface::printAST(lb);

            std::cout << indent << "ub:" << std::endl;
            SageInterface::printAST(ub);

            std::cout << indent << "step:" << std::endl;
            SageInterface::printAST(step);
        }

        // If is_canonical is true, then need to check
        // 1. Whether increment step c is a compile time constant
        // 2. The value of the induction variable is not changed in the loop
        if (is_canonical)
        {
            if (debug)
            {
                std::cout << RC::get_indent(1) << "is_canonical=True" << std::endl;
            }

            // We want to check whether increment step c is a compile time constant.
            // For that to be true, step must be a constant integer value, so it must be a SgIntVal type
            // TODO: support constant propagation and folding
            SgIntVal *step_value = isSgIntVal(step);
            bool is_increment_step_constexpr = (step_value != nullptr);

            if (is_increment_step_constexpr)
            {
                if (debug)
                {
                    const std::string indent = RC::get_indent(2);
                    std::cout << indent << "is_increment_step_constexpr=True, step_value=" << step_value->get_value() << std::endl;

                    if (verbose)
                    {
                        std::cout << indent << "body:" << std::endl;
                        SageInterface::printAST(body);
                    }
                }

                // Check whether the induction variable is modified in the subtree
                // with the root that is the loop body of for_loop_node
                std::set<SgInitializedName *> read_vars;
                std::set<SgInitializedName *> write_vars;
                SageInterface::collectReadWriteVariables(body, read_vars, write_vars);
                if (debug)
                {
                    const std::string indent = RC::get_indent(2);
                    if (verbose)
                    {
                        std::cout << indent << "body read:" << std::endl;
                        for (auto r : read_vars)
                        {
                            SageInterface::printAST(r);
                        }
                    }
                    std::cout << indent << "body write:" << std::endl;
                    for (auto w : write_vars)
                    {
                        SageInterface::printAST(w);
                    }
                }

                bool is_induction_variable_unmodified = (write_vars.count(ivar) == 0);
                if (is_induction_variable_unmodified)
                {
                    if (debug)
                    {
                        std::cout << RC::get_indent(3) << "is_induction_variable_unmodified=true" << std::endl;
                    }

                    std::cout << "Analyzable! " << RC::to_string(for_loop_node) << " with " << RC::to_string(ivar) << std::endl;
                    return ivar;
                }
            }
        }

        std::cout << "Not analyzable! " << RC::to_string(for_loop_node) << std::endl;
        return nullptr;
    }

    std::optional<DDTP> formulate_ddtp(const RawDDTP &raw_ddtp, const std::unordered_map<SgForStatement *, SgInitializedName *> &analyzable_loops)
    {
        std::vector<SgInitializedName *> common_induction_vars;

        // Construct common induction vars from inner to outer
        for (SgForStatement *for_stmt : raw_ddtp.outer_for_stmts)
        {
            if (auto fit = analyzable_loops.find(for_stmt); fit != analyzable_loops.end())
            {
                common_induction_vars.push_back(fit->second);
            }
            else
            {
                break;
            }
        }

        // Reverse common induction vars from outer to inner
        std::reverse(common_induction_vars.begin(), common_induction_vars.end());

        if (common_induction_vars.empty())
        {
            return std::nullopt;
        }

        return DDTP{raw_ddtp.w_array_ref, raw_ddtp.target_array_ref, std::move(common_induction_vars)};
    }

    std::optional<RawDDTP> is_potential_dependence_target_pair(
        SgPntrArrRefExp *w_array_ref,
        SgPntrArrRefExp *target_array_ref,
        SgScopeStatement *scope_stmt,
        bool debug = false, int indent = 0)
    {
        // Find write-xx dependence on the same array name
        SgInitializedName *w_array_name = get_array_name_from_ref(w_array_ref);
        SgInitializedName *target_array_name = get_array_name_from_ref(target_array_ref, debug, indent);
        if (target_array_name != w_array_name)
        {
            return std::nullopt;
        }
        if (debug)
        {
            std::cout << RC::get_indent(indent + 1) << "Name Matching.." << std::endl;
        }

        // Check target_array_ref is inside a for loop
        SgForStatement *w_array_ref_enclosing_for_stmt = get_enclosing_for_stmt(w_array_ref);
        ROSE_ASSERT(w_array_ref_enclosing_for_stmt);
        SgForStatement *target_array_ref_enclosing_for_stmt = get_enclosing_for_stmt(target_array_ref);
        if (target_array_ref_enclosing_for_stmt == nullptr)
        {
            return std::nullopt;
        }

        // Find common ancestor
        SgForStatement *ancestor_for_stmt = find_common_ancestor_for_stmt(w_array_ref_enclosing_for_stmt, target_array_ref_enclosing_for_stmt, scope_stmt);
        if (debug)
        {
            std::cout << RC::get_indent(indent + 1) << "Common Ancestor.." << std::endl;
            std::cout << RC::get_indent(indent + 2) << RC::to_string(ancestor_for_stmt) << std::endl;
        }

        // Find all outer loops
        std::vector<SgForStatement *> outer_for_stmts = get_outer_for_stmts(ancestor_for_stmt, scope_stmt);
        if (debug)
        {
            std::cout << RC::get_indent(indent + 1) << "Outer for_stmts=" << outer_for_stmts.size() << ".." << std::endl;
            for (SgForStatement *for_stmt : outer_for_stmts)
            {
                std::cout << RC::get_indent(indent + 2) << RC::to_string(for_stmt) << std::endl;
            }
        }

        return RawDDTP{w_array_ref, target_array_ref, std::move(outer_for_stmts)};
    }

    // Check over an entire scope. Do not call this function on multiple scopes that overlap
    DDTPCollection determine_potential_dependence_targets_of_scope(SgScopeStatement *scope_stmt,
                                                                   const std::unordered_map<SgForStatement *, SgInitializedName *> &analyzable_loops,
                                                                   bool debug = false)
    {
        std::vector<DDTP> ww_ddtps;
        std::vector<DDTP> wr_ddtps;

        // Get all read write refs
        std::vector<SgNode *> read_refs;
        std::vector<SgNode *> write_refs;
        SageInterface::collectReadWriteRefs(scope_stmt, read_refs, write_refs);

        // Filter only array refs
        auto mapfilter_SgPntrArrRefExp = [](const std::vector<SgNode *> &v)
        {
            std::vector<SgPntrArrRefExp *> tmp;
            std::transform(v.begin(), v.end(), std::back_inserter(tmp), [](SgNode *n)
                           { return isSgPntrArrRefExp(n); });
            std::vector<SgPntrArrRefExp *> res;
            std::copy_if(tmp.begin(), tmp.end(), std::back_inserter(res), [](SgPntrArrRefExp *n)
                         { return n; });
            return res;
        };
        std::vector<SgPntrArrRefExp *> read_array_refs = mapfilter_SgPntrArrRefExp(read_refs);
        std::vector<SgPntrArrRefExp *> write_array_refs = mapfilter_SgPntrArrRefExp(write_refs);

        if (debug)
        {
            std::cout << std::endl;
            print_ast(scope_stmt);
            std::cout << "In scope: " << RC::to_string(scope_stmt) << std::endl;
            std::cout << "read_array_refs:" << std::endl;
            for (auto r : read_array_refs)
            {
                SageInterface::printAST(r);
            }
            std::cout << "write_array_refs:" << std::endl;
            for (auto w : write_array_refs)
            {
                SageInterface::printAST(w);
            }
        }

        // Deal with write dependence
        for (auto w_array_ref_it = write_array_refs.begin(); w_array_ref_it != write_array_refs.end(); ++w_array_ref_it)
        {
            SgPntrArrRefExp *w_array_ref = *w_array_ref_it;
            if (debug)
            {
                std::cout << "Analyzing Write " << RC::to_string(w_array_ref) << std::endl;
                get_array_name_from_ref(w_array_ref, true, 1);
            }

            // Check w_array_ref is inside a for loop
            if (get_enclosing_for_stmt(w_array_ref) == nullptr)
                continue;

            // Deal with write-write dependence
            for (auto target_w_array_ref_it = w_array_ref_it + 1; target_w_array_ref_it != write_array_refs.end(); ++target_w_array_ref_it)
            {
                SgPntrArrRefExp *target_w_array_ref = *target_w_array_ref_it;
                if (debug)
                {
                    std::cout << RC::get_indent(1) << "Write Target " << RC::to_string(target_w_array_ref) << std::endl;
                }

                if (std::optional<RawDDTP> res = is_potential_dependence_target_pair(w_array_ref, target_w_array_ref, scope_stmt, debug, 2))
                {
                    if (std::optional<DDTP> ddtp_opt = formulate_ddtp(*res, analyzable_loops))
                    {
                        if (debug)
                        {
                            std::cout << RC::get_indent(1) << *ddtp_opt << std::endl;
                        }
                        ww_ddtps.emplace_back(std::move(*ddtp_opt));
                    }
                }
            }

            // Deal with write-read dependence
            for (SgPntrArrRefExp *target_r_array_ref : read_array_refs)
            {
                if (debug)
                {
                    std::cout << RC::get_indent(1) << "Read Target " << RC::to_string(target_r_array_ref) << std::endl;
                }

                if (std::optional<RawDDTP> res = is_potential_dependence_target_pair(w_array_ref, target_r_array_ref, scope_stmt, debug, 2))
                {
                    if (std::optional<DDTP> ddtp_opt = formulate_ddtp(*res, analyzable_loops))
                    {
                        if (debug)
                        {
                            std::cout << RC::get_indent(1) << *ddtp_opt << std::endl;
                        }
                        wr_ddtps.emplace_back(std::move(*ddtp_opt));
                    }
                }
            }
        }
        return {std::move(ww_ddtps), std::move(wr_ddtps)};
    }

    void process_function_body(SgFunctionDefinition *defn, bool debug)
    {
        SgBasicBlock *body = defn->get_body();
        std::cout << "Found a function" << std::endl;
        std::cout << "  " << defn->get_declaration()->get_qualified_name() << std::endl;
        // SageInterface::printAST(func);

        // Get all loops in the current function body
        Rose_STL_Container<SgNode *> loops = NodeQuery::querySubTree(body, V_SgForStatement);
        if (loops.size() == 0)
            return;

        // Build a mapping between analyzable loop and its indunction variable
        std::unordered_map<SgForStatement *, SgInitializedName *> analyzable_loops;
        for (Rose_STL_Container<SgNode *>::iterator iter = loops.begin(); iter != loops.end(); iter++)
        {
            SgNode *current_loop = *iter;

            std::cout << std::endl;
            std::cout << "Found a loop" << std::endl;
            SgInitializedName *ind_var = is_loop_analyzable(current_loop, debug);

            if (ind_var)
            {
                SgForStatement *for_stmt = isSgForStatement(current_loop);
                ROSE_ASSERT(for_stmt);
                analyzable_loops.emplace(for_stmt, ind_var);
            }
        }

        // Determine dependence check targets
        DDTPCollection ddtpc = determine_potential_dependence_targets_of_scope(body, analyzable_loops, debug);
        std::cout << std::endl;
        std::cout << "========================== BEGIN ========================" << std::endl;
        std::cout << RC::to_string(defn) << std::endl;
        std::cout << "Data Dependence Testing Pair Collection" << std::endl;
        std::cout << ddtpc;
        std::cout << "==========================  END  ========================" << std::endl;
    }
}
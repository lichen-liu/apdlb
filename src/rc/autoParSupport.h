#ifndef auto_par_support_INCLUDED
#define auto_par_support_INCLUDED

// Common headers for a ROSE translator
//#include <rose.h>
#include "LivenessAnalysis.h"
#include "CommandOptions.h"

// OpenMP attribute for OpenMP 3.0
#include "OmpAttribute.h"

// Array Annotation headers
#include <CPPAstInterface.h>
#include <ArrayAnnot.h>
#include <ArrayRewrite.h>

// Dependence graph headers
#include <AstInterface_ROSE.h>
#include <LoopTransformInterface.h>
#include <AnnotCollect.h>
#include <OperatorAnnotation.h>
#include <LoopTreeDepComp.h>

// Variable classification support
#include "DefUseAnalysis.h"

// Other standard C++ headers
#include <vector>
#include <string>
#include <map>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace AutoParallelization
{
    // Handle annotation, debugging flags, no longer used, replaced by using Sawyer command line processing
    //   void autopar_command_processing(std::vector<std::string>&argvList);

    // Required analysis and their initialization
    extern DFAnalysis *defuse;
    extern LivenessAnalysis *liv;

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

    // Conduct necessary analyses on the project, can be called multiple times during program transformations.
    bool initialize_analysis(SgProject *project = nullptr, bool debug = false);

    //----------------------end of the cost modeling section ------------------------
    // Release the resources for analyses
    void release_analysis();

    // Return the loop invariant of a canonical loop, return nullptr otherwise
    SgInitializedName *getLoopInvariant(SgNode *loop);

    // Compute dependence graph for a loop, using ArrayInterface and ArrayAnnoation
    LoopTreeDepGraph *ComputeDependenceGraph(SgNode *loop, ArrayInterface *, ArrayAnnotation *annot);

    // Get the live-in and live-out variable sets for a for loop, recomputing liveness analysis if requested (useful after program transformation)
    void GetLiveVariables(SgNode *loop, std::vector<SgInitializedName *> &liveIns,
                          std::vector<SgInitializedName *> &liveOuts, bool reCompute = false);

    // Collect visible referenced variables within a scope (mostly a loop).
    // Ignoring local variables declared within the scope.
    // Specially recognize nested loops' invariant variables, which are candidates for private variables
    void CollectVisibleVaribles(SgNode *loop, std::vector<SgInitializedName *> &resultVars, std::vector<SgInitializedName *> &loopInvariants, bool scalarOnly = false);

    //! Collect a loop's variables which cause any kind of dependencies. Consider scalars only if requested.
    void CollectVariablesWithDependence(SgNode *loop, LoopTreeDepGraph *depgraph, std::vector<SgInitializedName *> &resultVars, bool scalarOnly = false);

    // Variable classification for a loop node
    // Collect private, firstprivate, lastprivate, reduction and save into attribute
    void AutoScoping(SgNode *sg_node, OmpSupport::OmpAttribute *attribute, LoopTreeDepGraph *depgraph);

    // Recognize reduction variables for a loop
    // Refactored into SageInterface
    // std::vector<SgInitializedName*> RecognizeReduction(SgNode *sg_node, OmpSupport::OmpAttribute* attribute, std::vector<SgInitializedName*>& candidateVars);

    std::vector<SgInitializedName *> CollectUnallowedScopedVariables(OmpSupport::OmpAttribute *attribute);

    // Collect all classified variables from an OmpAttribute attached to a loop node,regardless their omp type
    std::vector<SgInitializedName *> CollectAllowedScopedVariables(OmpSupport::OmpAttribute *attribute);

    // Eliminate irrelevant dependencies for a loop node 'sg_node'
    // Save the remaining dependencies which prevent parallelization
    void DependenceElimination(SgNode *sg_node, LoopTreeDepGraph *depgraph, std::vector<DepInfo> &remain, OmpSupport::OmpAttribute *attribute,
                               std::map<SgNode *, bool> &indirectTable, ArrayInterface *array_interface = 0, ArrayAnnotation *annot = 0);

    // Parallelize an input loop at its outermost loop level, return true if successful
    bool ParallelizeOutermostLoop(SgNode *loop, ArrayInterface *array_interface, ArrayAnnotation *annot);

    //! Check if two expressions access different memory locations. If in double, return false
    // This is helpful to exclude some dependence relations involving two obvious different memory location accesses
    // TODO: move to SageInterface when ready
    bool differentMemoryLocation(SgExpression *e1, SgExpression *e2);

    //! Check if a loop has any unsupported language features so we can skip them for now
    bool useUnsupportedLanguageFeatures(SgNode *loop, VariantT *blackConstruct);

} // end namespace

#endif // auto_par_support_INCLUDED

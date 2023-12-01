#ifndef VSGOPENMW_VSGUTIL_COMPUTEBIN_H
#define VSGOPENMW_VSGUTIL_COMPUTEBIN_H

#include <vsg/nodes/Bin.h>
#include <vsg/app/CommandGraph.h>

namespace vsgUtil
{
    /*
     * ComputeBin is a bin class that uses a nested command graph to reorder the execution
     * of commands that can't run during a RenderPass, e.g. compute commands.
     * The submitOrder of the nested command graph is used to control whether the
     * Bin's contents should execute before or after the parent command graph.
     * ComputeBin is useful to encapsulate compute commands within a rendering scene graph, and inherit any associated masks,
     * so that the compute commands are switched off automatically when rendering graphs are switched off or detached from the scene graph.
     */
    class ComputeBin : public vsg::Inherit<vsg::Bin, ComputeBin>
    {
    public:
        // Note, restoring of matrix and state stacks is currently not done.
        void traverse(vsg::RecordTraversal& visitor) const override;

        // Users need to create and set up the CommandGraph with device, queueFamily and submitOrder.
        // During traverse(RecordTraversal&), the Bin's contents are automatically assigned to commandGraph->children.
        vsg::ref_ptr<vsg::CommandGraph> commandGraph;
    };
}

#endif

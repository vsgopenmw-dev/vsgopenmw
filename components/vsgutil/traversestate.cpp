#include "traversestate.hpp"

#include <vsg/nodes/StateGroup.h>

namespace vsgUtil
{
    void TraverseState::apply(vsg::StateGroup& sg)
    {
        for (auto& sc : sg.stateCommands)
            sc->accept(*this);
        apply(static_cast<vsg::Object&>(sg));
    }
}

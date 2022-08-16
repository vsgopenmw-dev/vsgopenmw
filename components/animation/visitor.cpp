#include "visitor.hpp"

#include <vsg/nodes/StateGroup.h>

#include "tags.hpp"

namespace Anim
{
    void Visitor::apply(vsg::Object &controlled)
    {
        if (auto aux = controlled.getAuxiliary())
        {
            for (auto &[key, obj] : aux->userObjects)
            {
                if (key == Controller::sAttachKey)
                    apply(static_cast<Controller&>(*obj), controlled);
                else if (key == Tags::sAttachKey)
                    apply(static_cast<Tags&>(*obj));
            }
        }
        controlled.traverse(*this);
    }

    void Visitor::apply(vsg::StateGroup &sg)
    {
        for (auto &sc : sg.stateCommands)
            sc->accept(*this);
        apply(static_cast<vsg::Object&>(sg));
    }
}

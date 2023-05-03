#include "visitor.hpp"

#include <vsg/state/DescriptorBuffer.h>

#include "tags.hpp"

namespace Anim
{
    void Visitor::apply(vsg::Object& controlled)
    {
        if (auto aux = controlled.getAuxiliary())
        {
            for (auto& [key, obj] : aux->userObjects)
            {
                if (key == Controller::sAttachKey)
                    apply(static_cast<Controller&>(*obj), controlled);
                else if (key == Tags::sAttachKey)
                    apply(static_cast<Tags&>(*obj));
            }
        }
        controlled.traverse(*this);
    }

    void Visitor::apply(vsg::DescriptorBuffer& d)
    {
        for (auto& bi : d.bufferInfoList)
            if (bi->data)
                apply(*bi->data);
    }
}

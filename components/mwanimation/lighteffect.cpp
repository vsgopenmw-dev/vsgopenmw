#include "lighteffect.hpp"

#include <vsg/lighting/Light.h>
#include <vsg/lighting/PointLight.h>
#include <vsg/nodes/Group.h>

#include <components/vsgutil/removechild.hpp>

#include "object.hpp"

namespace
{
    static const std::string lightEffect = "lightfx";
}
namespace MWAnim
{
    void setLightEffect(MWAnim::Object& obj, float intensity)
    {
        auto auxNode = obj.node();
        auto group = obj.nodeToAddChildrenTo();
        if (intensity == 0)
        {
            if (!auxNode->getAuxiliary())
                return;
            if (auto light = auxNode->getObject<vsg::PointLight>(lightEffect))
            {
                vsgUtil::removeChild(group, light);
                auxNode->removeObject(lightEffect);
            }
        }
        else
        {
            auto light = auxNode->getObject<vsg::PointLight>(lightEffect);
            if (!light)
            {
                auto l = vsg::PointLight::create();
                auxNode->setObject(lightEffect, l);
                light = l;
                group->addChild(l);
                // light->colorType = Ambient;
                light->color = vsg::vec3(1.5, 1.5, 1.5);
            }
            light->intensity = intensity;
        }
    }
}

#include "lighteffect.hpp"

#include <vsg/nodes/Light.h>

#include <components/vsgutil/removechild.hpp>

namespace
{
    static const std::string lightEffect = "lightfx";
}
namespace MWAnim
{
    void setLightEffect(vsg::Group &node, float intensity)
    {
        if (intensity == 0)
        {
            if (!node.getAuxiliary())
                return;
            if (auto light = node.getObject<vsg::PointLight>(lightEffect))
            {
                vsgUtil::removeChild(&node, light);
                node.removeObject(lightEffect);
            }
        }
        else
        {
            auto light = node.getObject<vsg::PointLight>(lightEffect);
            if (!light)
            {
                auto l = vsg::PointLight::create();
                node.setObject(lightEffect, l);
                light = l;
                node.addChild(l);
                //light->colorType = Ambient;
                light->color = vsg::vec3(1.5,1.5,1.5);
            }
            light->intensity = intensity;
        }
    }
}

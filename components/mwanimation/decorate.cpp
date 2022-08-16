#include "decorate.hpp"

#include <vsg/maths/transform.h>

#include <components/animation/context.hpp>
#include <components/animation/transform.hpp>
#include <components/vsgutil/addchildren.hpp>

#include "light.hpp"

namespace MWAnim
{
    PlaceholderResult decorate(vsg::ref_ptr<vsg::Node> &node, const ESM::Light *light, bool mirror)
    {
        auto result = clonePlaceholdersIfRequired(node);
        if (!result.contents.contains(Anim::Contents::Skins) && (result.placeholders.boneOffset || light || mirror))
        {
            auto t = vsg::ref_ptr{new Anim::Transform};
            if (auto boneOffset = result.placeholders.boneOffset)
                t->translation = boneOffset->translation;
            if (light)
                t->setAttitude(vsg::quat(vsg::radians(-90.f), vsg::vec3(1,0,0)));
            if (mirror)
                t->scale = {-1,1,1};
            vsgUtil::addChildren(*t, *node);
            node = t;
        }
        if (light)
        {
            vsg::ref_ptr<vsg::Group> fallback;
            auto attachLight = result.placeholders.attachLight;
            if (!attachLight)
            {
                if (auto group = dynamic_cast<vsg::Group*>(node.get()))
                    fallback = group;
                else
                {
                    fallback = vsg::Group::create();
                    vsgUtil::addChildren(*fallback, *node);
                    node = fallback;
                }
            }
            addLight(attachLight, fallback, *light, result.controllers);
        }
        return result;
    }
}

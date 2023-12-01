#include "scene.hpp"

#include <vsg/maths/transform.h>

#include <components/pipeline/viewbindings.hpp>

namespace View
{
    Scene::Scene()
        : DynamicDescriptorValue<Pipeline::Data::Scene>(Pipeline::Descriptors::VIEW_SCENE_BINDING)
    {
        auto& v = value();
        v.time = 0;
        v.fogStart = v.fogScale = 0;
    }

    void Scene::setLightPosition(const vsg::vec3& pos, const vsg::mat4& invView)
    {
        value().lightViewPos = vsg::vec4(vsg::normalize(pos) * invView, 0);
        dirty();
    }
}

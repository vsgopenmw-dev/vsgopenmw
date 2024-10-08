#ifndef VSGOPENMW_MWRENDER_ENV_H
#define VSGOPENMW_MWRENDER_ENV_H

#include <optional>

#include <vsg/nodes/StateGroup.h>
#include <vsg/state/DescriptorImage.h>

namespace MWWorld
{
    class ConstPtr;
}
namespace MWRender
{
    /*
     * Adds reflective glow.
     */
    vsg::ref_ptr<vsg::Descriptor> readEnv(vsg::ref_ptr<const vsg::Options> options);
    vsg::ref_ptr<vsg::StateGroup> createEnv(const vsg::vec4& color, float alpha = 1.f);
    void addEnv(vsg::ref_ptr<vsg::Node>& node, std::optional<vsg::vec4> color, float alpha = 1.f);
    void addEnv(vsg::ref_ptr<vsg::Node>& node, const MWWorld::ConstPtr& item, float alpha = 1.f);
    std::optional<vsg::vec4> getGlowColor(const MWWorld::ConstPtr& item);
}

#endif

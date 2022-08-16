#ifndef VSGOPENMW_VIEW_LIGHTGRID_H
#define VSGOPENMW_VIEW_LIGHTGRID_H

#include <vsg/state/BufferedDescriptorBuffer.h>

#include <components/vsgutil/composite.hpp>

namespace View
{
    /*
     * Narrows light selection.
     */
    class LightGrid : public vsgUtil::Composite<vsg::Node>
    {
        vsg::ref_ptr<vsg::BufferedDescriptorBuffer> mIndexCount;
    public:
        LightGrid(const vsg::Options &shaderOptions, vsg::ref_ptr<vsg::DescriptorSet> viewSet);
        void update();
    };
}

#endif

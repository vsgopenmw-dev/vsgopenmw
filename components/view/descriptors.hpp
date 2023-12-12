#ifndef VSGOPENMW_VIEW_DESCRIPTORS_H
#define VSGOPENMW_VIEW_DESCRIPTORS_H

#include <vsg/state/DescriptorSet.h>
#include <vsg/core/Data.h>

namespace View
{
    /*
     * Creates descriptors required for lighting calculations.
     * @param compute Optionally supports light grid assigment compute shader.
     */
    vsg::Descriptors createLightingDescriptors(vsg::ref_ptr<vsg::Data> lightData, bool compute);

    /*
     * Creates valid, but ineffective descriptors.
     */
    vsg::ref_ptr<vsg::Descriptor> dummyEnvMap();
    vsg::ref_ptr<vsg::Descriptor> dummyShadowMap();
    vsg::ref_ptr<vsg::Descriptor> dummyShadowSampler();

    /*
     * Creates VIEW_SET descriptor set.
     * @param descriptors All the descriptors statically used by associated pipelines.
     */
    vsg::ref_ptr<vsg::DescriptorSet> createViewDescriptorSet(const vsg::Descriptors& descriptors);
}

#endif

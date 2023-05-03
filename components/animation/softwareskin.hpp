#ifndef VSGOPENMW_ANIMATION_SOFTWARESKIN_H
#define VSGOPENMW_ANIMATION_SOFTWARESKIN_H

#include <vsg/core/Array.h>

#include <components/vsgutil/arraystate.hpp>

#include "skin.hpp"

namespace Anim
{
    /*
     * Supports vertex traversal.
     */
    class SoftwareSkin : public vsgUtil::ArrayState<SoftwareSkin>
    {
        vsg::ref_ptr<const vsg::mat4Array> mBoneMatrices;
        vsg::ref_ptr<Skin> mSkin;

    public:
        SoftwareSkin(vsg::ref_ptr<const vsg::mat4Array> boneMatrices, vsg::ref_ptr<Skin> skin);
        void apply(const vsg::vec3Array& vertices) override;
    };
}

#endif

#ifndef VSGOPENMW_ANIMATION_SOFTWARESKIN_H
#define VSGOPENMW_ANIMATION_SOFTWARESKIN_H

#include <vsg/state/ArrayState.h>

namespace Anim
{
    class Skin;

    /*
     * Supports vertex traversal.
     */
    class SoftwareSkin : public vsg::ArrayState
    {
        const vsg::mat4Array &mBoneMatrices;
        Skin &mSkin;
    public:
        SoftwareSkin(const SoftwareSkin &skin, const ArrayState &arrayState);
        SoftwareSkin(const vsg::mat4Array &boneMatrices, Skin &skin);
        vsg::ref_ptr<vsg::ArrayState> clone(vsg::ref_ptr<vsg::ArrayState> arrayState) override;
        void apply(const vsg::vec3Array &vertices) override;
    };
}

#endif

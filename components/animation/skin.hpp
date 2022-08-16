#ifndef VSGOPENMW_ANIMATION_SKIN_H
#define VSGOPENMW_ANIMATION_SKIN_H

#include <vsg/core/Array.h>

#include "copydata.hpp"

namespace Anim
{
    class Transform;
    //using TransformPath = std::vector<vsg::Transform*>;
    using TransformPath = std::vector<vsg::ref_ptr<const Transform>>;
    struct BoneData
    {
        TransformPath path; // Automatically set
        vsg::mat4 invBindMatrix;
        std::string name;
    };

    /*
     * Accumulates bone matrices.
     */
    class Skin : public MCopyData<Skin, vsg::mat4Array>
    {
    public:
        Skin();
        std::vector<BoneData> bones;
        vsg::ref_ptr<vsg::vec4Array> boneIndices;
        vsg::ref_ptr<vsg::vec4Array> boneWeights;
        TransformPath skinPath;
        vsg::mat4 inverseSkinPath() const;
        void apply(vsg::mat4Array &array, float);
        void link(Context &context, vsg::Object &) override;
    };
}

#endif

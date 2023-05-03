#ifndef VSGOPENMW_ANIMATION_SKIN_H
#define VSGOPENMW_ANIMATION_SKIN_H

#include <optional>

#include <vsg/core/Array.h>

#include "updatedata.hpp"

namespace Anim
{
    struct BoneData
    {
        vsg::mat4 invBindMatrix;
        vsg::sphere bounds;
        std::string name;
    };
    class Transform;

    /*
     * Accumulates bone matrices.
     */
    class Skin : public MUpdateData<Skin, vsg::mat4Array>
    {
        void optimizePaths();
        /*
         * Automatically updates bounds of untransformed parent cull node.
         */
        void updateBounds();
        class PrivateBoneData;
        std::vector<PrivateBoneData> mBones;
        std::vector<const Transform*> mReferenceFrame;
        vsg::dsphere* mDynamicBounds{};
        vsg::dsphere* mDynamicDepthSortedBounds{};
    public:
        Skin();
        ~Skin();
        std::vector<BoneData> bones;
        vsg::ref_ptr<vsg::vec4Array> boneIndices;
        vsg::ref_ptr<vsg::vec4Array> boneWeights;
        std::optional<vsg::mat4> transform;
        void apply(vsg::mat4Array& array, float);
        /*
         * Links to parent cull node and StateGroup.
         */
        void link(Context& context, vsg::Object&) override;
    };
}

#endif

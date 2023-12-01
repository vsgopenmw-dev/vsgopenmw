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
        /*vsg::sphere*/vsg::dsphere bounds;
        std::string name;
    };
    class Transform;

    /*
     * Controller class for accumulating bone matrices for use by skinning shaders.
     * The link(..) method will find the parent Cull/DepthSorted and StateGroup nodes, for updating the bounds automatically, and setting up a SoftwareSkin object for the StateGroup to use for intersections on the CPU.
     */
    class Skin : public MUpdateData<Skin, vsg::mat4Array>
    {
        void optimizePaths();
        /*
         * Automatically updates bounds of untransformed parent Cull/DepthSorted nodes.
         */
        void updateBounds();
        class PrivateBoneData;
        std::vector<PrivateBoneData> mBones;
        std::vector<const Transform*> mReferenceFrame;
        std::vector<vsg::dsphere*> mDynamicBounds;
    public:
        Skin();
        ~Skin();
        /*
         * Bone information set by users.
         */
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

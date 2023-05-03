#include "skin.hpp"

#include <iostream>

#include <vsg/maths/transform.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/CullGroup.h>
#include <vsg/nodes/DepthSorted.h>

#include <components/vsgutil/computetransform.hpp>
#include <components/vsgutil/nodepath.hpp>
#include <components/vsgutil/bounds.hpp>

#include "bones.hpp"
#include "context.hpp"
#include "softwareskin.hpp"
#include "transform.hpp"
#include "visitor.hpp"

namespace
{
    vsg::mat4 inverseRotationAndTranslation(const vsg::mat4& m)
    {
        //return transpose3x3(m) * translate(-m.trans)
        return {
            m[0][0], m[1][0], m[2][0], 0,
            m[0][1], m[1][1], m[2][1], 0,
            m[0][2], m[1][2], m[2][2], 0,
            -(m[3][0] * m[0][0] + m[3][1] * m[0][1] + m[3][2] * m[0][2]), -(m[3][0] * m[1][0] + m[3][1] * m[1][1] + m[3][2] * m[1][2]), -(m[3][0] * m[2][0] + m[3][1] * m[2][1] + m[3][2] * m[2][2]), 1 };
    }
}
namespace Anim
{
    class GetSkinPath : public Anim::Visitor
    {
        vsg::Object* mTarget{};
        vsg::dsphere* mCurrentCullBounds{};
        vsg::dsphere* mCurrentDepthSortedBounds{};

    public:
        vsgUtil::SearchPath<const Transform*> transformPath;
        vsgUtil::SearchPath<vsg::StateGroup*> statePath;

        GetSkinPath(vsg::Object* target)
            : mTarget(target)
        {
            overrideMask = vsg::MASK_ALL;
        }
        vsg::dsphere* foundCullBounds{};
        vsg::dsphere* foundDepthSortedBounds{};
        using Visitor::apply;
        void apply(vsg::Transform& transform) override
        {
            mCurrentCullBounds = {};
            mCurrentDepthSortedBounds = {};
            transformPath.accumulateCastAndTraverse(transform, *this);
        }
        void apply(vsg::DepthSorted& node) override
        {
            mCurrentDepthSortedBounds = &node.bound;
            transformPath.traverseNode(node, *this);
            mCurrentDepthSortedBounds = {};
        }
        void apply(vsg::CullNode& node) override
        {
            mCurrentCullBounds = &node.bound;
            transformPath.traverseNode(node, *this);
            mCurrentCullBounds = {};
        }
        void apply(vsg::CullGroup& node) override
        {
            mCurrentCullBounds = &node.bound;
            transformPath.traverseNode(node, *this);
            mCurrentCullBounds = {};
        }
        void apply(vsg::StateGroup& sg) override
        {
            auto ppn = statePath.pushPop(&sg);
            TraverseState::apply(sg);
        }
        void apply(Controller& ctrl, vsg::Object& target) override
        {
            if (&target == mTarget)
            {
                transformPath.foundPath = transformPath.path;
                statePath.foundPath = statePath.path;
                foundCullBounds = mCurrentCullBounds;
                foundDepthSortedBounds = mCurrentDepthSortedBounds;
            }
        }
    };

    struct Skin::PrivateBoneData
    {
        std::vector<const Transform*> path;
        vsg::mat4 matrix;
        vsg::mat4 invBindMatrix;
    };

    Skin::Skin()
    {
        hints.autoPlay = true;
    }

    Skin::~Skin() {}

    void Skin::apply(vsg::mat4Array& array, float)
    {
        std::optional<vsg::mat4> skinMatrix;
        if (!mReferenceFrame.empty())
        {
            // skinMatrix = vsg::inverse_4x3(vsgUtil::computeTransform(mReferenceFrame));
            // openmw-6294-skin-nitrishape-scale
            skinMatrix = inverseRotationAndTranslation(vsgUtil::computeTransform(mReferenceFrame));
        }
        if (transform)
        {
            if (skinMatrix)
                skinMatrix = (*transform) * (*skinMatrix);
            else
                skinMatrix = (*transform);
        }

        for (size_t i = 0; i < mBones.size(); ++i)
        {
            const auto& boneData = mBones[i];
            mBones[i].matrix = vsgUtil::computeTransform(boneData.path);
            if (skinMatrix)
                mBones[i].matrix = (*skinMatrix) * mBones[i].matrix;
            array.at(i) = mBones[i].matrix * boneData.invBindMatrix;
        }
        updateBounds();
    }

    void Skin::link(Context& context, vsg::Object& target)
    {
        if (!context.bones)
        {
            std::cerr << "!context.bones" << std::endl;
            return;
        }

        mReferenceFrame = vsgUtil::extract<const Transform>(context.attachmentPath);            GetSkinPath visitor(&target);
        context.attachmentPath.back()->accept(visitor);
        auto& addPath = visitor.transformPath.foundPath;
        if (!addPath.empty())
        {
            if (mReferenceFrame.back() == addPath.front())
                mReferenceFrame.pop_back();
            vsgUtil::addPath(mReferenceFrame, addPath);
        }

        if (!visitor.statePath.foundPath.empty())
            visitor.statePath.foundPath.back()->prototypeArrayState = new SoftwareSkin(vsg::ref_ptr{ target.cast<const vsg::mat4Array>() }, vsg::ref_ptr{this});
        else
            std::cerr << "!GetSkinPath(" << this << ")" << std::endl;

        mBones.clear();
        mBones.reserve(bones.size());
        for (auto& bone : bones)
        {
            auto& inserted = mBones.emplace_back();
            if (auto b = context.bones->search(bone.name))
                inserted.path = vsgUtil::path<const Transform*>(b->path);
            else
                std::cerr << bone.name << " bone.path.empty()" << std::endl;
            inserted.invBindMatrix = bone.invBindMatrix;
        }

        optimizePaths();

        mDynamicBounds = visitor.foundCullBounds;
        mDynamicDepthSortedBounds = visitor.foundDepthSortedBounds;
    }

    void Skin::optimizePaths()
    {
        /*
         * Trims paths to their lowest common ancestor.
         */
        size_t trimCount = 0;
        for (auto& node : mReferenceFrame)
        {
            auto itr = mBones.begin();
            for (; itr != mBones.end(); ++itr)
            {
                if (itr->path.size() < trimCount || itr->path[trimCount] != node)
                    break;
            }
            if (itr != mBones.end())
                break;
            ++trimCount;
        }
        if (trimCount > 0)
        {
            vsgUtil::trim(mReferenceFrame, trimCount);
            for (auto& bone : mBones)
                vsgUtil::trim(bone.path, trimCount);
        }
    }

    void Skin::updateBounds()
    {
        if (mDynamicBounds)
        {
            vsg::box boundingBox;
            for (size_t i = 0; i < mBones.size(); ++i)
            {
                auto& matrix = mBones[i].matrix;
                auto boneBounds = vsgUtil::transformSphere(matrix, bones[i].bounds);
                vsgUtil::expandBoxBySphere(boundingBox, boneBounds);
            }
            auto sphere = vsgUtil::toSphere(boundingBox);
            mDynamicBounds->set(sphere.center, sphere.radius);
            if (mDynamicDepthSortedBounds)
                mDynamicDepthSortedBounds->set(sphere.center, sphere.radius);
        }
    }
}

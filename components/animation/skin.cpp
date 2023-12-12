#include "skin.hpp"

#include <iostream>

#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/CullGroup.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/utils/ComputeBounds.h>

#include <components/vsgutil/computetransform.hpp>
#include <components/vsgutil/nodepath.hpp>
#include <components/vsgutil/bounds.hpp>

#include "bones.hpp"
#include "context.hpp"
#include "softwareskin.hpp"
#include "transform.hpp"
#include "visitor.hpp"

namespace Anim
{
    namespace
    {
        vsg::mat4 computeSkinMatrix(const std::vector<const Transform*>& path)
        {
            vsg::mat4 matrix;
            for (auto& node : path)
            {
                // matrix = vsg::translate(-node->translation) * matrix;
                for (unsigned i = 0; i < 3; ++i)
                {
                    float tmp = node->translation[i];
                    matrix[0][i] -= tmp*matrix[0][3];
                    matrix[1][i] -= tmp*matrix[1][3];
                    matrix[2][i] -= tmp*matrix[2][3];
                    matrix[3][i] -= tmp*matrix[3][3];
                }
                // invRotation = transpose(node.rotation);
                vsg::mat4 invRotation = {
                    node->rotation[0][0], node->rotation[1][0], node->rotation[2][0], 0,
                    node->rotation[0][1], node->rotation[1][1], node->rotation[2][1], 0,
                    node->rotation[0][2], node->rotation[1][2], node->rotation[2][2], 0,
                    0, 0, 0, 1
                };
                matrix = invRotation * matrix;
            }
            return matrix;
        }

        class GetSkinPath : public Anim::Visitor
        {
            vsg::Object* mTarget{};
            std::vector<vsg::dsphere*> mCurrentNodeBounds{};

        public:
            vsgUtil::SearchPath<const Anim::Transform*> transformPath;
            vsgUtil::SearchPath<vsg::StateGroup*> statePath;

            GetSkinPath(vsg::Object* target)
                : mTarget(target)
            {
                overrideMask = vsg::MASK_ALL;
            }
            std::vector<vsg::dsphere*> foundNodeBounds;
            using Visitor::apply;
            void apply(vsg::Transform& transform) override
            {
                if (!mCurrentNodeBounds.empty())
                {
                    //if (childContainsSkin)
                    //   std::cerr << "!GetSkinPath::apply(vsg::Transform&): transforming bounds is not implemented, ignoring transform node." << std::endl;
                }
                transformPath.accumulateCastAndTraverse(transform, *this);
            }
            void apply(vsg::DepthSorted& node) override
            {
                mCurrentNodeBounds.push_back(&node.bound);
                transformPath.traverseNode(node, *this);
                mCurrentNodeBounds.pop_back();
            }
            void apply(vsg::CullNode& node) override
            {
                mCurrentNodeBounds.push_back(&node.bound);
                transformPath.traverseNode(node, *this);
                mCurrentNodeBounds.pop_back();
            }
            void apply(vsg::CullGroup& node) override
            {
                mCurrentNodeBounds.push_back(&node.bound);
                transformPath.traverseNode(node, *this);
                mCurrentNodeBounds.pop_back();
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
                    foundNodeBounds = mCurrentNodeBounds;
                }
            }
        };
    }

    struct Skin::PrivateBoneData
    {
        std::vector<const Transform*> path;
        vsg::mat4 matrix;
        vsg::mat4 invBindMatrix;
    };

    Skin::Skin() {}

    Skin::~Skin() {}

    void Skin::apply(vsg::mat4Array& array, float)
    {
        // skinMatrix = vsg::inverse_4x3(vsgUtil::computeTransform(mReferenceFrame));
        // openmw-6294-skin-nitrishape-scale
        vsg::mat4 skinMatrix = computeSkinMatrix(mReferenceFrame);
        if (transform)
            skinMatrix = (*transform) * skinMatrix;

        for (size_t i = 0; i < mBones.size(); ++i)
        {
            mBones[i].matrix = skinMatrix * vsgUtil::computeTransform(mBones[i].path);
            array.at(i) = mBones[i].matrix * mBones[i].invBindMatrix;
        }

        if (!mDynamicBounds.empty())
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
            visitor.statePath.foundPath.back()->prototypeArrayState = SoftwareSkin::create(vsg::ref_ptr{ target.cast<const vsg::mat4Array>() }, vsg::ref_ptr{this});
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

        mDynamicBounds = visitor.foundNodeBounds;
        if (mDynamicBounds.empty())
            std::cerr << "!GetSkinPath::foundNodeBounds" << std::endl;
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
                if (itr->path.size() <= trimCount || itr->path[trimCount] != node)
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
        vsg::ComputeBounds compute;
        for (size_t i = 0; i < mBones.size(); ++i)
        {
            auto& matrix = mBones[i].matrix;
            compute.matrixStack = { vsg::dmat4(matrix) };
            compute.add(bones[i].bounds);
        }
        auto sphere = vsgUtil::toSphere(compute.bounds);
        for (auto& bound : mDynamicBounds)
            bound->set(sphere.center, sphere.radius);
    }
}

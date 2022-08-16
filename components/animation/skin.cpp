#include "skin.hpp"

#include <iostream>

#include <vsg/maths/transform.h>
#include <vsg/nodes/StateGroup.h>

#include <components/vsgutil/computetransform.hpp>

#include "visitor.hpp"
#include "context.hpp"
#include "bones.hpp"
#include "transform.hpp"
#include "softwareskin.hpp"

namespace
{
    template <class Src, class Dst>
    void copyPath(const Src &src, Dst &dst)
    {
        for (auto &i : src)
            dst.emplace_back(i);
    }
}

namespace Anim
{
    class LinkSkin : public Visitor
    {
        Skin &mSkin;
        TransformPath mPath;
        vsg::StateGroup *mCurrentStateGroup{};
    public:
        LinkSkin(Skin &skin) : mSkin(skin)
        {
            overrideMask = vsg::MASK_ALL;
        }
        TransformPath skinPath;
        using Visitor::apply;
        void apply(vsg::Transform &transform) override
        {
            if (auto *trans = dynamic_cast<Anim::Transform*>(&transform))
            {
                mPath.emplace_back(trans);
                transform.traverse(*this);
                mPath.pop_back();
            }
            else
                transform.traverse(*this);
        }
        void apply(vsg::StateGroup &sg) override
        {
            mCurrentStateGroup = &sg;
            Anim::Visitor::apply(sg);
        }
        void apply(Controller &ctrl, vsg::Object &o) override
        {
            if (&ctrl == &mSkin)
            {
                skinPath = mPath;
                auto &boneMatrices = static_cast<vsg::mat4Array&>(*static_cast<vsg::DescriptorBuffer&>(o).bufferInfoList[0]->data);
                mCurrentStateGroup->prototypeArrayState = new SoftwareSkin(boneMatrices, mSkin);
            }
        }
    };

    Skin::Skin()
    {
        hints.autoPlay = true;
    }

    void Skin::apply(vsg::mat4Array &array, float)
    {
        auto getBoneMatrix = [this](size_t i) -> vsg::mat4
        {
            const auto &boneData = bones[i];
            return vsgUtil::computeTransform(boneData.path) * boneData.invBindMatrix;
        };
        for (size_t i=0; i<bones.size(); ++i)
            array.at(i) = getBoneMatrix(i);
        if (!skinPath.empty())
        {
            auto skinMat = inverseSkinPath();
            for (size_t i=0; i<bones.size(); ++i)
                array.at(i) = skinMat * array.at(i);
        }
    }

    vsg::mat4 Skin::inverseSkinPath() const
    {
        return vsg::inverse_4x3(vsgUtil::computeTransform(skinPath));
    }

    void Skin::link(Context &context, vsg::Object &)
    {
        if (!context.bones)
        {
            std::cerr << "!context.bones" << std::endl;
            return;
        }

        LinkSkin visitor(*this);
        context.scene->accept(visitor);
        /*if (!context.attachmentPath.empty())
            copyPath(context.attachmentPath, skinPath);
        else*/
            skinPath = visitor.skinPath;

        for (auto &bone : bones)
        {
            if (auto b = context.bones->search(bone.name))
                copyPath(b->path, bone.path);
            else
                std::cerr << bone.name << " bone.path.empty()" << std::endl;
        }
    }
}

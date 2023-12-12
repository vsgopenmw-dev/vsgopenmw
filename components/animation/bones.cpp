#include "bones.hpp"

#include <stack>

#include <components/vsgutil/name.hpp>
#include <components/vsgutil/traverse.hpp>
#include <components/vsgutil/nodepath.hpp>

#include "transform.hpp"

namespace Anim
{
    class CopyBones : public vsgUtil::AccumulatePath<vsg::ref_ptr<Transform>>, public vsgUtil::TConstTraverse<vsg::Node>
    {
        Bones& mBones;
        const std::map<std::string, uint32_t>& mGroups;
        std::stack<uint32_t> mGroupStack;

    public:
        CopyBones(Bones& bones, const std::map<std::string, uint32_t>& groups)
            : mBones(bones)
            , mGroups(groups)
        {
            overrideMask = vsg::MASK_ALL;
            mGroupStack.emplace(0);
        }
        using vsg::ConstVisitor::apply;
        void apply(const vsg::Transform& transform) override
        {
            auto trans = dynamic_cast<const Anim::Transform*>(&transform);
            if (!trans)
            {
                transform.traverse(*this);
                return;
            }
            auto cloned = vsg::ref_ptr{ new Transform(*trans) };
            cloned->children.clear();
            auto ppn = pushPop(cloned);
            auto name = vsgUtil::getName(transform);
            bool pushedGroup = false;
            if (!name.empty())
            {
                name = mBones.normalizeFunc(name);
                auto group = mGroups.find(name);
                if (group != mGroups.end())
                {
                    mGroupStack.push(group->second);
                    pushedGroup = true;
                }
                mBones.mByName.emplace(std::move(name), Bone{ this->path, mGroupStack.top() });
            }
            transform.traverse(*this);
            if (pushedGroup)
                mGroupStack.pop();
        }
    };

    class InitBones : public vsgUtil::AccumulatePath<vsg::ref_ptr<Transform>>, public vsgUtil::TTraverse<vsg::Node>
    {
        Bones& mBones;

    public:
        InitBones(Bones& bones)
            : mBones(bones)
        {
            overrideMask = vsg::MASK_ALL;
        }
        using vsg::Visitor::apply;
        void apply(vsg::Transform& transform) override
        {
            auto trans = dynamic_cast<Anim::Transform*>(&transform);
            if (!trans)
            {
                transform.traverse(*this);
                return;
            }
            auto ppn = pushPop(trans);
            auto name = vsgUtil::getName(transform);
            if (!name.empty())
            {
                name = mBones.normalizeFunc(name);
                mBones.mByName.emplace(std::move(name), Bone{ .path = this->path });
            }
            transform.traverse(*this);
        }
    };

    Bones::Bones()
        : attached(false)
    {
    }

    Bones::~Bones() {}

    Bones::Bones(NormalizeFunc f, const vsg::Node& graphToCopy, const std::map<std::string, uint32_t>& groups)
        : attached(false)
        , normalizeFunc(f)
    {
        CopyBones visitor(*this, groups);
        graphToCopy.accept(visitor);
    }

    Bones::Bones(NormalizeFunc f, vsg::Node& scene)
        : attached(true)
        , normalizeFunc(f)
    {
        InitBones visitor(*this);
        scene.accept(visitor);
    }

    Bone* Bones::search(const std::string& name)
    {
        auto it = mByName.find(normalizeFunc(name));
        return it != mByName.end() ? &it->second : nullptr;
    }
}

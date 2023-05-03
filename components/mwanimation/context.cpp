#include "context.hpp"

#include <vsg/io/read.h>

#include <components/animation/controllermap.hpp>
#include <components/vsgutil/readnode.hpp>
#include <components/vsgutil/compilecontext.hpp>

namespace MWAnim
{
    Context::~Context() {}

    const std::string& Context::pickSkeleton(bool firstPerson, bool isFemale, bool isBeast, bool isWerewolf) const
    {
        auto& f = firstPerson ? firstPersonFiles : files;
        if (isWerewolf)
            return f.wolfskin;
        else if (isBeast)
            return f.baseanimkna;
        else if (isFemale)
            return f.baseanimfemale;
        else
            return f.baseanim;
    }

    vsg::ref_ptr<vsg::Node> Context::readEffect(const std::string& fname) const
    {
        return vsgUtil::readNode(fname, effectOptions);
    }

    vsg::ref_ptr<vsg::Node> Context::readNode(const std::string& fname) const
    {
        return vsgUtil::readNode(fname, nodeOptions);
    }

    vsg::ref_ptr<vsg::Node> Context::readActor(const std::string& fname) const
    {
        return vsgUtil::readNode(fname, actorOptions);
    }

    vsg::ref_ptr<Anim::ControllerMap> Context::readAnimation(const std::string& file) const
    {
        return vsg::read_cast<Anim::ControllerMap>(file, animOptions);
    }

    std::vector<vsg::ref_ptr<Anim::ControllerMap>> Context::readAnimations(const std::vector<std::string>& files) const
    {
        std::vector<vsg::ref_ptr<Anim::ControllerMap>> ret;
        for (auto& file : files)
        {
            if (auto anim = readAnimation(file))
                ret.emplace_back(anim);
        }
        return ret;
    }
}

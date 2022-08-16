#include "context.hpp"

#include <iostream>

#include <vsg/viewer/CompileManager.h>
#include <vsg/core/Exception.h>
#include <vsg/io/read.h>

#include <components/vsgutil/readnode.hpp>
#include <components/animation/controllermap.hpp>

namespace MWAnim
{
    const std::string &Context::pickSkeleton(bool firstPerson, bool isFemale, bool isBeast, bool isWerewolf) const
    {
        auto &f = firstPerson ? firstPersonFiles : files;
        if (isWerewolf)
            return f.wolfskin;
        else if (isBeast)
            return f.baseanimkna;
        else if (isFemale)
            return f.baseanimfemale;
        else
            return f.baseanim;
    }

    bool Context::compile(vsg::ref_ptr<vsg::Node> node) const
    {
        try
        {
            auto res = compileManager->compile(node);
            return res.result == VK_SUCCESS;
        }
        catch (vsg::Exception &e)
        {
            std::cerr << "!compile(" << e.message << ")" <<std::endl;
            return false;
        }
    }

    vsg::ref_ptr<vsg::Node> Context::readNode(const std::string &fname, bool throw_) const
    {
        return vsgUtil::readNode(fname, nodeOptions, throw_);
    }

    vsg::ref_ptr<Anim::ControllerMap> Context::readAnimation(const std::string &file) const
    {
        return vsg::read_cast<Anim::ControllerMap>(file, animOptions);
    }

    std::vector<vsg::ref_ptr<Anim::ControllerMap>> Context::readAnimations(const std::vector<std::string> &files) const
    {
        std::vector<vsg::ref_ptr<Anim::ControllerMap>> ret;
        for (auto &file : files)
        {
            if (auto anim = readAnimation(file))
                ret.emplace_back(anim);
        }
        return ret;
    }
}

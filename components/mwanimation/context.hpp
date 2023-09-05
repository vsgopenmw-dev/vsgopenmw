#ifndef VSGOPENMW_MWANIMATION_CONTEXT_H
#define VSGOPENMW_MWANIMATION_CONTEXT_H

#include <vsg/io/Options.h>
#include <vsg/core/ref_ptr.h>

#include <components/animation/mask.hpp>

namespace Anim
{
    class ControllerMap;
}
namespace vsgUtil
{
    class CompileContext;
}
namespace MWAnim
{
    /*
     * Configures scene graph creation.
     */
    struct Context
    {
        ~Context();

        struct Files
        {
            std::string wolfskin;
            std::string baseanim;
            std::string baseanimkna;
            std::string baseanimfemale;
        } files;

        Files firstPersonFiles;

        const std::string& pickSkeleton(bool firstPerson, bool female, bool beast, bool werewolf) const;

        Anim::Mask mask{};

        vsg::ref_ptr<const vsg::Options> nodeOptions;
        vsg::ref_ptr<const vsg::Options> effectOptions;
        vsg::ref_ptr<const vsg::Options> actorOptions;
        vsg::ref_ptr<const vsg::Options> textureOptions;
        std::vector<vsg::ref_ptr<const vsg::Options>> partBoneOptions;
        vsg::ref_ptr<const vsg::Options> animOptions;

        vsg::ref_ptr<vsgUtil::CompileContext> compileContext;

        vsg::ref_ptr<vsg::Node> readNode(const std::string& fname) const;
        vsg::ref_ptr<vsg::Node> readEffect(const std::string& fname) const;
        vsg::ref_ptr<vsg::Node> readActor(const std::string& fname) const;
        vsg::ref_ptr<Anim::ControllerMap> readAnimation(const std::string& file) const;
        std::vector<vsg::ref_ptr<Anim::ControllerMap>> readAnimations(const std::vector<std::string>& files) const;
        void prune() const;
    };
}

#endif

#ifndef VSGOPENMW_MWANIMATION_CONTEXT_H
#define VSGOPENMW_MWANIMATION_CONTEXT_H

#include <vsg/io/Options.h>

#include <components/animation/mask.hpp>

namespace vsg
{
    class CompileManager;
}
namespace Anim
{
    class ControllerMap;
}
namespace MWAnim
{
    /*
     * Configures scene graph creation.
     */
    struct Context
    {
        struct Files
        {
        std::string wolfskin;
        std::string baseanim;
        std::string baseanimkna;
        std::string baseanimfemale;
        } files;

        Files firstPersonFiles;

        const std::string &pickSkeleton(bool firstPerson, bool female, bool beast, bool werewolf) const;

        Anim::Mask mask{};
        //Bins bins;

        std::function<std::string(const std::string&)> findActorModelCallback;

        vsg::ref_ptr<const vsg::Options> nodeOptions;
        vsg::ref_ptr<const vsg::Options> textureOptions;
        std::vector<vsg::ref_ptr<const vsg::Options>> partBoneOptions;
        vsg::ref_ptr<const vsg::Options> animOptions;

        vsg::CompileManager *compileManager{};

        bool compile(vsg::ref_ptr<vsg::Node> node) const;

        vsg::ref_ptr<vsg::Node> readNode(const std::string &fname, bool throw_ = true) const;

        vsg::ref_ptr<Anim::ControllerMap> readAnimation(const std::string &file) const;
        std::vector<vsg::ref_ptr<Anim::ControllerMap>> readAnimations(const std::vector<std::string> &files) const;
    };
}

#endif

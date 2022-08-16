#include "animcontext.hpp"

#include <components/settings/settings.hpp>
#include <components/animation/mask.hpp>
#include <components/mwanimation/partbones.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/pipeline/override.hpp>

#include "mask.hpp"

namespace
{
    std::vector<vsg::ref_ptr<const vsg::Options>> partBoneOptions(const vsg::Options &base)
    {
        std::vector<vsg::ref_ptr<const vsg::Options>> ret(ESM::PRT_Count);
        for (int i=0; i<ESM::PRT_Count; ++i)
        {
            auto options = vsg::Options::create(base);
            auto &bonename = MWAnim::sPartBones.at(static_cast<ESM::PartReferenceType>(i));
            if (bonename.find("Left") != std::string::npos)
            {
                auto override_ = vsg::ref_ptr{new Pipeline::Override};
                override_->composite([](Pipeline::Options &o) {
                   if (!o.hasMode(Pipeline::GeometryMode::SKIN))
                       o.cullMode = VK_CULL_MODE_FRONT_BIT;
                   }, *options);
            }
            // PRT_Hair seems to be the only type that breaks consistency and uses a filter that's different from the attachment bone
            options->setValue("skinfilter", (i == ESM::PRT_Hair) ? "hair" : bonename);
            ret[i] = options;
        }
        return ret;
    }
}
namespace MWRender
{
    MWAnim::Context animContext(Resource::ResourceSystem *resourceSystem, vsg::CompileManager *in_compile, const vsg::Options *baseNodeOptions/*,Settings::Manager &*/)
    {
        if (!baseNodeOptions)
            baseNodeOptions = resourceSystem->nodeOptions;
        return {
            .files={
                .wolfskin = Settings::Manager::getString("wolfskin", "Models"),
                .baseanim = Settings::Manager::getString("baseanim", "Models"),
                .baseanimkna = Settings::Manager::getString("baseanimkna", "Models"),
                .baseanimfemale = Settings::Manager::getString("baseanimfemale", "Models")},
            .firstPersonFiles={
                .wolfskin = Settings::Manager::getString("wolfskin1st", "Models"),
                .baseanim = Settings::Manager::getString("baseanim1st", "Models"),
                .baseanimkna = Settings::Manager::getString("baseanimkna1st", "Models"),
                .baseanimfemale = Settings::Manager::getString("baseanimfemale1st", "Models")},
            .mask = {Mask_Particle},
            .findActorModelCallback = [resourceSystem] (const std::string &path) -> std::string { return Misc::ResourceHelpers::correctActorModelPath(path, resourceSystem->getVFS()); },
            .nodeOptions = vsg::ref_ptr{baseNodeOptions},
            .textureOptions = resourceSystem->textureOptions,
            .partBoneOptions = partBoneOptions(*baseNodeOptions),
            .animOptions = resourceSystem->animationOptions,
            .compileManager = in_compile
        };
    }
}

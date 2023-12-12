#include "animcontext.hpp"

#include <vsg/utils/SharedObjects.h>

#include <components/animation/mask.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/mwanimation/partbones.hpp>
#include <components/pipeline/override.hpp>
#include <components/pipeline/material.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/settings.hpp>

#include "mask.hpp"

namespace
{
    vsg::ref_ptr<vsg::Options> actorOptions(Resource::ResourceSystem* resourceSystem)
    {
        auto options = vsg::Options::create(*resourceSystem->nodeOptions);
        options->sharedObjects = vsg::SharedObjects::create();
        options->findFileCallback = [resourceSystem](const vsg::Path& file, const vsg::Options* options) -> vsg::Path
        {
            std::string path = "meshes/" + file;
            return vsg::Path(Misc::ResourceHelpers::correctActorModelPath(path, resourceSystem->getVFS()));
        };
        return options;
    }

    vsg::ref_ptr<vsg::Options> effectOptions(const vsg::Options& base)
    {
        auto options = vsg::Options::create(base);
        options->sharedObjects = vsg::SharedObjects::create();
        auto override_ = vsg::ref_ptr{ new Pipeline::Override };
        override_->materialData = [](Pipeline::Data::Material& m) {
            // Morrowind has a white ambient light attached to the root VFX node of the scenegraph
            m.emissive += vsg::vec4(m.ambient.x, m.ambient.y, m.ambient.z, 0);
        };
        override_->composite(*options);
        return options;
    }

    std::vector<vsg::ref_ptr<const vsg::Options>> partBoneOptions(const vsg::Options& base)
    {
        std::vector<vsg::ref_ptr<const vsg::Options>> ret(ESM::PRT_Count);
        for (int i = 0; i < ESM::PRT_Count; ++i)
        {
            auto options = vsg::Options::create(base);
            options->sharedObjects = vsg::SharedObjects::create();
            auto& bonename = MWAnim::sPartBones.at(static_cast<ESM::PartReferenceType>(i));
            if (bonename.find("Left") != std::string::npos)
            {
                auto override_ = vsg::ref_ptr{ new Pipeline::Override };
                override_->pipelineOptions = [](Pipeline::Options& o) {
                    if (!o.shader.hasMode(Pipeline::Mode::SKIN))
                        o.frontFace = VK_FRONT_FACE_CLOCKWISE;
                };
                override_->composite(*options);
            }
            // PRT_Hair seems to be the only type that breaks consistency and uses a filter that's different from the
            // attachment bone
            options->setValue("skinfilter", (i == ESM::PRT_Hair) ? "hair" : bonename);
            ret[i] = options;
        }
        return ret;
    }
}
namespace MWRender
{
    MWAnim::Context animContext(Resource::ResourceSystem* resourceSystem, vsg::ref_ptr<vsgUtil::CompileContext> in_compile,
        const vsg::Options* baseNodeOptions /*,Settings::Manager &*/)
    {
        if (!baseNodeOptions)
            baseNodeOptions = resourceSystem->nodeOptions;
        return { .files = { .wolfskin = Settings::Manager::getString("wolfskin", "Models"),
                     .baseanim = Settings::Manager::getString("baseanim", "Models"),
                     .baseanimkna = Settings::Manager::getString("baseanimkna", "Models"),
                     .baseanimfemale = Settings::Manager::getString("baseanimfemale", "Models") },
            .firstPersonFiles = { .wolfskin = Settings::Manager::getString("wolfskin1st", "Models"),
                //.baseanim = Settings::Manager::getString("baseanim1st", "Models"),
                .baseanim = "xbase_anim.1st.nif",
                .baseanimkna = Settings::Manager::getString("baseanimkna1st", "Models"),
                .baseanimfemale = Settings::Manager::getString("baseanimfemale1st", "Models") },
            .mask = { Mask_Particle },
            .nodeOptions = vsg::ref_ptr{ baseNodeOptions },
            .effectOptions = effectOptions(*baseNodeOptions),
            .actorOptions = actorOptions(resourceSystem),
            .textureOptions = resourceSystem->textureOptions,
            .partBoneOptions = partBoneOptions(*baseNodeOptions),
            .animOptions = resourceSystem->animationOptions,
            .compileContext = in_compile };
    }
}

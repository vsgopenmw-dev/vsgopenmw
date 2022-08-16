#include "override.hpp"

#include <vsg/state/DescriptorBuffer.h>

namespace Pipeline
{
    const std::string Override::sAttachKey = "override";

    Override::Override()
    {
    }

    Override::~Override()
    {
    }

    void Override::composite(std::function<void(Options&)> p, vsg::Options &options)
    {
        auto existing = get(options);
        if (!material && existing && existing->material)
            material = existing->material;
        if (existing && existing->pipelineOptions)
        {
            auto existingFunc = existing->pipelineOptions;
            pipelineOptions = [existingFunc, p] (Options &o) { existingFunc(o); p(o); };
        }
        else
            pipelineOptions = p;
        attachTo(options);
    }
}

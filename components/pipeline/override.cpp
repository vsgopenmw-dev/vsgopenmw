#include "override.hpp"

#include <vsg/state/DescriptorBuffer.h>

namespace
{
    template <class Object>
    std::function<void(Object&)> composite(std::function<void(Object&)> a, std::function<void(Object&)> b)
    {
        if (a && b)
        {
            return [a, b](Object& o) {
                a(o);
                b(o);
            };
        }
        else if (a)
            return a;
        else
            return b;
    }
}
namespace Pipeline
{
    const std::string Override::sAttachKey = "override";

    Override::Override() {}

    Override::~Override() {}

    void Override::composite(vsg::Options& options)
    {
        auto existing = get(options);
        if (!existing)
        {
            attachTo(options);
            return;
        }
        auto newOverride = vsg::ref_ptr{ new Override };
        newOverride->material = material;
        newOverride->pipelineOptions = ::composite(existing->pipelineOptions, pipelineOptions);
        newOverride->materialData = ::composite(existing->materialData, materialData);
        newOverride->attachTo(options);
    }
}

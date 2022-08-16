#ifndef VSGOPENMW_PIPELINE_OVERRIDE_H
#define VSGOPENMW_PIPELINE_OVERRIDE_H

#include <vsg/io/Options.h>

#include <components/vsgutil/attachable.hpp>

#include "options.hpp"

namespace Pipeline
{
    /*
     * Requests custom pipeline.
     */
    class Override : public vsgUtil::Attachable<Override, vsg::Options>
    {
    public:
        Override();
        ~Override();
        void composite(std::function<void(Options&)> pipelineOptions, vsg::Options &options);
        static const std::string sAttachKey;
        std::function<void(Options&)> pipelineOptions;
        vsg::ref_ptr<vsg::DescriptorBuffer> material;
    };
}

#endif

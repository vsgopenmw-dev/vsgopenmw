#ifndef VSGOPENMW_PIPELINE_OVERRIDE_H
#define VSGOPENMW_PIPELINE_OVERRIDE_H

#include <vsg/io/Options.h>

#include <components/vsgutil/attachable.hpp>

#include "options.hpp"

namespace Pipeline
{
    namespace Data
    {
        class Material;
    }

    /*
     * Requests custom pipeline.
     */
    class Override : public vsgUtil::Attachable<Override, vsg::Options>
    {
    public:
        Override();
        ~Override();
        /*
         * Preserves existing overrides in options.
         */
        void composite(vsg::Options& options);
        static const std::string sAttachKey;
        std::function<void(Options&)> pipelineOptions;
        std::function<void(Pipeline::Data::Material&)> materialData;
        vsg::ref_ptr<vsg::DescriptorBuffer> material;
    };
}

#endif

#ifndef VSGOPENMW_VSGADAPTERS_NIF_NIF_H
#define VSGOPENMW_VSGADAPTERS_NIF_NIF_H

#include <vsg/io/ReaderWriter.h>

namespace Pipeline
{
    class Builder;
}

namespace vsgAdapters
{
    /*
     * Reads VER_MW nif file into vsg::Node or throws.
     * Depends on nif, animation, pipeline.
     */
    //template<int NifVersion>
    class nif : public vsg::ReaderWriter
    {
        const Pipeline::Builder &mBuilder;
        vsg::ref_ptr<const vsg::Options> mImageOptions;
    public:
        nif(const Pipeline::Builder &builder, vsg::ref_ptr<const vsg::Options> imageOptions) : mBuilder(builder), mImageOptions(imageOptions) {}
        vsg::ref_ptr<vsg::Object> read(std::istream&, vsg::ref_ptr<const vsg::Options> = {}) const override;

        bool getFeatures(Features& features) const override;

        /// Whether or not nodes marked as "MRK" should be shown.
        /// These should be hidden ingame, but visible in the editor.
        bool showMarkers = false;

        // Mask to use for nodes that ignore the crosshair intersection.
        // This is used for NiCollisionSwitch nodes with NiCollisionSwitch state set to disabled.
        unsigned int intersectionDisabledMask = ~0;
    };
}

#endif

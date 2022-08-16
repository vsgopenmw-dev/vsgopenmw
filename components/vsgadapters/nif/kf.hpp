#ifndef VSGOPENMW_VSGADAPTERS_NIF_KF_H
#define VSGOPENMW_VSGADAPTERS_NIF_KF_H

#include <vsg/io/ReaderWriter.h>

namespace vsgAdapters
{
    /*
     * Reads VER_MW kf file into Anim::ControllerMap or throws.
     * Depends on nif, animation.
     */
    class kf : public vsg::ReaderWriter
    {
    public:
        vsg::ref_ptr<vsg::Object> read(std::istream&, vsg::ref_ptr<const vsg::Options> = {}) const override;
        bool getFeatures(Features& features) const override;
    };
}

#endif


#ifndef VSGOPENMW_MWRENDER_SHADOW_H
#define VSGOPENMW_MWRENDER_SHADOW_H

#include <components/view/shadow.hpp>

namespace MWRender
{
    /*
     * Optionally configures shadows.
     */
    class Shadow
    {
        vsg::ref_ptr<vsg::Node> mDepthPass;
        float mFar;

    public:
        Shadow(vsg::Context& ctx /*, Settings::Manager&*/);
        ~Shadow();
        vsg::Mask viewMask() const;
        vsg::Descriptor* shadowMap();
        std::unique_ptr<View::Shadow> shadow;
    };
}

#endif

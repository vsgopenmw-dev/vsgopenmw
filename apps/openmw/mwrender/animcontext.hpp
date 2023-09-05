#ifndef VSGOPENMW_MWRENDER_ANIMCONTEXT_H
#define VSGOPENMW_MWRENDER_ANIMCONTEXT_H

#include <components/mwanimation/context.hpp>
#include <components/vsgutil/compilecontext.hpp>

namespace Resource
{
    class ResourceSystem;
}
namespace MWRender
{
    MWAnim::Context animContext(Resource::ResourceSystem*, vsg::ref_ptr<vsgUtil::CompileContext> in_compile,
        const vsg::Options* baseNodeOptions = {} /*, Settings::Manager &*/);
}

#endif

#ifndef VSGOPENMW_MWRENDER_ANIMCONTEXT_H
#define VSGOPENMW_MWRENDER_ANIMCONTEXT_H

#include <components/mwanimation/context.hpp>

namespace Resource
{
    class ResourceSystem;
}
namespace MWRender
{
    MWAnim::Context animContext(Resource::ResourceSystem *, vsg::CompileManager *in_compile, const vsg::Options *baseNodeOptions={}/*, Settings::Manager &*/);
}

#endif

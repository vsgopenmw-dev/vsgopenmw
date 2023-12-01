#ifndef VSGOPENMW_VSGUTIL_COMPILECONTEXT_H
#define VSGOPENMW_VSGUTIL_COMPILECONTEXT_H

#include <functional>

#include <vsg/core/Object.h>
#include <vsg/core/Mask.h>

namespace vsg
{
    class Context;
    class CompileManager;
    class CompileResult;
    class Object;
    class Device;
}
namespace vsgUtil
{
    /*
     * Controls Vulkan object lifetime.
     * @note CompileManager contains a thread safe queue of available compile traversals.
     * @note Multiple threads compiling unrelated objects may safely use independent CompileManagers.
     * @note Multiple threads compiling overlapping objects must use a shared CompileManager with numCompileTraversals set to 1.
     */
    class CompileContext : public vsg::Object
    {
    public:
        CompileContext();
        ~CompileContext();
        vsg::ref_ptr<CompileContext> clone(vsg::Mask viewMask) const;
        vsg::Device* device{};
        vsg::ref_ptr<vsg::CompileManager> compileManager;

        /*
         * Optionally selects views and render passes that pipelines should be compiled for.
         */
        std::function<bool(vsg::Context&)> contextSelectionFunction;

        /*
         * Signifies that object has been attached to the scene graph and requires Vulkan objects to be compiled.
         */
        std::function<void(const vsg::CompileResult&)> onCompiled;
        bool compile(vsg::ref_ptr<vsg::Object> obj) const;

        /*
         * Signifies that object has been detached from the scene graph and its Vulkan objects are no longer required after this frame.
         */
        std::function<void(vsg::ref_ptr<vsg::Object>)> onDetached;
        void detach(vsg::ref_ptr<vsg::Object> obj) const  { onDetached(obj); }

        /*
         * Conveniently assigns contextSelectionFunction for views with this mask.
         */
        void setViewMask(vsg::Mask mask);
    };
}

#endif

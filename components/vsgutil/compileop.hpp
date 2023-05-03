#ifndef VSGOPENMW_VSGUTIL_COMPILEOP_H
#define VSGOPENMW_VSGUTIL_COMPILEOP_H

//#include <chrono>

#include "operation.hpp"
#include "compilecontext.hpp"

namespace vsgUtil
{
    struct CompileOp : public vsgUtil::Operation
    {
        vsg::ref_ptr<vsgUtil::CompileContext> context;
        vsg::ref_ptr<vsg::Object> object;
        CompileOp(vsg::ref_ptr<vsg::Object> in_obj, vsg::ref_ptr<vsgUtil::CompileContext> in_context) : object(in_obj), context(in_context) {}
        void operate() override
        {
            //auto start = std::chrono::steady_clock::now();
            if (!context->compile(object))
                throw std::runtime_error("!CompileOp");
            /*
            auto end = std::chrono::steady_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
            std::cout << "CompileOp(" << object->className() << ")::run() = " << dt << "s" << std::endl;
            */
        }
    };
}

#endif

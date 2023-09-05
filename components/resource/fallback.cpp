#include "fallback.hpp"

#include <iostream>

#include <vsg/io/read.h>

namespace Resource
{
    vsg::ref_ptr<vsg::Object> Fallback::read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
    {
        /*
         * Prevents recursion.
         */
        if (std::find(fallbackFiles.begin(), fallbackFiles.end(), filename.string()) != fallbackFiles.end())
            return {};

        if (auto obj = readFallback(options))
        {
            std::cerr << "Using fallback object for " << filename.c_str() << std::endl;
            return obj;
        }
        return {};
    }

    vsg::ref_ptr<vsg::Object> Fallback::readFallback(vsg::ref_ptr<const vsg::Options> options) const
    {
        for (auto& f : fallbackFiles)
        {
            if (auto obj = vsg::read(f, options))
                return obj;
        }
        return fallbackObject;
    }
}

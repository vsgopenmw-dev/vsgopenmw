#ifndef VSGOPENMW_MWRENDER_SCREENSHOTINTERFACE_H
#define VSGOPENMW_MWRENDER_SCREENSHOTINTERFACE_H

#include <functional>

#include <vsg/core/ref_ptr.h>

namespace vsg
{
    class Data;
}
namespace MWRender
{
    struct ScreenshotInterface
    {
        int framesUntilReady{};
        std::function<void(int w, int h)> request;
        std::function<std::pair<vsg::ref_ptr<vsg::Data>, bool>()> retrieve;
    };
}

#endif

#ifndef VSGOPENMW_VSGUTIL_SHAREDVIEW_H
#define VSGOPENMW_VSGUTIL_SHAREDVIEW_H

#include <vsg/viewer/View.h>

namespace vsgUtil
{
    /*
     * Reduces pipeline compilation for views with compatible render passes.
     */
    vsg::ref_ptr<vsg::View> createSharedView();
    vsg::ref_ptr<vsg::View> createSharedView(vsg::ref_ptr<vsg::Camera> camera, vsg::ref_ptr<vsg::Node> child);
}

#endif

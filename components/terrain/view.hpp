#ifndef VSGOPENMW_TERRAIN_VIEW_H
#define VSGOPENMW_TERRAIN_VIEW_H

#include <components/vsgutil/composite.hpp>

namespace Terrain
{
    /**
     * @brief A View is a collection of rendering objects that are visible from a given view point and distance.
     */
    class View : public vsgUtil::Composite<vsg::Node>
    {
    public:
        virtual ~View() {}

        static constexpr float reuseDistance = 150; // large value should be safe because the visibility of each node is still updated individually for each camera even if the base view was reused. this value also serves as a threshold for when a newly loaded LOD gets unloaded again so that if you hover around an LOD transition point the LODs won't keep loading and unloading all the time.
        static constexpr float sqrReuseDistance = reuseDistance * reuseDistance;
    };
}

#endif

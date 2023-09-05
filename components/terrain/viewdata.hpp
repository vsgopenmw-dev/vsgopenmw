#ifndef VSGOPENMW_TERRAIN_VIEWDATA_H
#define VSGOPENMW_TERRAIN_VIEWDATA_H

#include <vector>
#include <compare>

#include <vsg/nodes/Group.h>

#include "view.hpp"
#include "bounds.hpp"

namespace Terrain
{
    struct ViewDataEntry
    {
        bool set(const Bounds& in_bounds, float in_distance)
        {
            if (bounds == in_bounds)
                return false;
            else
            {
                bounds = in_bounds;
                distance = in_distance;
                return true;
            }
        }
        Bounds bounds;
        float distance = 0;
    };

    class ViewData : public View
    {
    public:
        ViewData()
        {
            mNode = rootNode = vsg::Group::create();
        }

        void add(const Bounds& bounds, float distance)
        {
            unsigned int index = numEntries++;

            if (index + 1 > entries.size())
                entries.resize(index + 1);

            ViewDataEntry& entry = entries[index];
            if (entry.set(bounds, distance))
                changed = true;
        }

        void reset()
        {
            // reset index for next frame
            numEntries = 0;
            changed = false;
        }

        Bounds getContainingChunk(const vsg::vec2& point) const
        {
            for (unsigned int i = 0; i < numEntries; ++i)
            {
                if (entries[i].bounds.contains(point))
                    return entries[i].bounds;
            }
            return {};
        }

        void clear()
        {
            numEntries = 0;
            entries.clear();
            rootNode->children.clear();
        }

        vsg::dvec3 viewPoint;
        float viewDistance = 0;

        /// Indicates at least one chunk of mEntries has changed.
        // @note Such changes may necessitate a revalidation of cached nodes elsewhere depending
        /// on the parameters that affect the creation of node.
        bool changed = false;

        std::vector<ViewDataEntry> entries;
        unsigned int numEntries = 0;

        vsg::ref_ptr<vsg::Group> rootNode;
    };
}

#endif

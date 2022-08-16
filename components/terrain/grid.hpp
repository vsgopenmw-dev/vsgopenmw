#ifndef VSGOPENMW_TERRAIN_GRID_H
#define VSGOPENMW_TERRAIN_GRID_H

#include <map>

#include "group.hpp"
#include "createlayers.hpp"

namespace Terrain
{

    /// @brief Simple terrain implementation that loads cells in a grid, with no LOD. Only requested cells are loaded.
    class Grid : public Group
    {
    public:
        Grid(Storage* storage, vsg::ref_ptr<const vsg::Options> options, vsg::ref_ptr<const vsg::Options> shaderOptions);

        void loadCell(int x, int y) override;
        void unloadCell(int x, int y) override;

    private:
        vsg::ref_ptr<vsg::Node> buildTerrain (float chunkSize, const vsg::vec2& chunkCenter);

        // split each ESM::Cell into mNumSplits*mNumSplits terrain chunks
        unsigned int mNumSplits = 4;

        std::map<std::pair<int,int>, vsg::ref_ptr<vsg::Node>> mGrid;
        CreateLayers mLayers;
    };
}

#endif

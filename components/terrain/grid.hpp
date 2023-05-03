#ifndef VSGOPENMW_TERRAIN_GRID_H
#define VSGOPENMW_TERRAIN_GRID_H

#include <map>

#include <components/vsgutil/cache.hpp>
#include <components/vsgutil/compilecontext.hpp>

#include "createlayers.hpp"
#include "group.hpp"

namespace Terrain
{

    /// @brief Simple terrain implementation that loads cells in a grid, with no LOD. Only requested cells are loaded.
    class Grid : public Group
    {
    public:
        Grid(
            Storage* storage, vsg::ref_ptr<vsgUtil::CompileContext> compile, vsg::ref_ptr<const vsg::Options> options, vsg::ref_ptr<const vsg::Options> shaderOptions, vsg::ref_ptr<vsg::Sampler> samplerOptions);
        ~Grid();

        void loadCell(int x, int y) override;
        void unloadCell(int x, int y) override;
        vsg::ref_ptr<vsgUtil::Operation> cacheCell(int x, int y) override;
        using Coord = std::pair<int, int>;
        vsg::ref_ptr<vsg::Node> create(const Coord&) const;

    private:
        vsg::ref_ptr<vsg::Node> buildTerrain(float chunkSize, const vsg::vec2& chunkCenter) const;

        // split each ESM::Cell into mNumSplits*mNumSplits terrain chunks
        unsigned int mNumSplits = 4;

        std::map<Coord, vsg::ref_ptr<vsg::Node>> mGrid;
        CreateLayers mLayers;
        vsg::ref_ptr<vsgUtil::CompileContext> mCompile;
        vsgUtil::RefCache<Coord, vsg::Node> mCache;
    };
}

#endif

#include "grid.hpp"

#include <vsg/nodes/QuadGroup.h>
#include <vsg/nodes/MatrixTransform.h>

#include <components/vsgutil/cullnode.hpp>
#include <components/vsgutil/removechild.hpp>

#include "storage.hpp"
#include "geometry.hpp"

namespace Terrain
{
    Grid::Grid(Storage* storage, vsg::ref_ptr<const vsg::Options> options, vsg::ref_ptr<const vsg::Options> shaderOptions)
        : Group(storage, options, shaderOptions)
        , mLayers(options, shaderOptions)
    {
        mNode = vsg::Group::create();
    }

    vsg::ref_ptr<vsg::Node> Grid::buildTerrain (float chunkSize, const vsg::vec2 &chunkCenter)
    {
        if (chunkSize * mNumSplits > 1.f)
        {
            // keep splitting
            auto quadGroup = vsg::QuadGroup::create();
            float newChunkSize = chunkSize/2.f;
            quadGroup->children = {
                buildTerrain(newChunkSize, chunkCenter + vsg::vec2(newChunkSize/2.f, newChunkSize/2.f)),
                buildTerrain(newChunkSize, chunkCenter + vsg::vec2(newChunkSize/2.f, -newChunkSize/2.f)),
                buildTerrain(newChunkSize, chunkCenter + vsg::vec2(-newChunkSize/2.f, newChunkSize/2.f)),
                buildTerrain(newChunkSize, chunkCenter + vsg::vec2(-newChunkSize/2.f, -newChunkSize/2.f))
            };
            return quadGroup;
        }
        else
        {
            auto sg = mLayers.create(*mStorage, chunkSize, chunkCenter);
            sg->addChild(createGeometry(*mStorage, chunkSize, chunkCenter, 0, 0));

            auto transform = vsg::MatrixTransform::create();
            transform->subgraphRequiresLocalFrustum = false;
            auto cellWorldSize = mStorage->cellWorldSize;
            transform->matrix[3] = {chunkCenter.x*cellWorldSize, chunkCenter.y*cellWorldSize, 0, 1};
            transform->addChild(sg);
            return vsgUtil::createCullNode(*transform);
        }
    }

    void Grid::loadCell(int x, int y)
    {
        if (mGrid.find(std::make_pair(x, y)) != mGrid.end())
            return;

        vsg::vec2 center(x+0.5f, y+0.5f);
        auto terrainNode = buildTerrain(1.f, center);
        if (!terrainNode)
            return;

        mNode->addChild(terrainNode);
        mGrid[std::make_pair(x,y)] = terrainNode;
    }

    void Grid::unloadCell(int x, int y)
    {
        auto it = mGrid.find(std::make_pair(x,y));
        if (it == mGrid.end())
            return;

        vsgUtil::removeChild(mNode, it->second);
        mGrid.erase(it);
    }
}

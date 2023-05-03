#include "grid.hpp"

#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/MatrixTransform.h>

#include <components/vsgutil/removechild.hpp>
#include <components/vsgutil/compileop.hpp>
#include <components/vsgutil/arraystate.hpp>
#include <components/vsgutil/bounds.hpp>
#include <components/vsgutil/cullnode.hpp>

#include "geometry.hpp"
#include "storage.hpp"

namespace Terrain
{
    /*
     * Converts heights to vertices for intersection testing on the CPU.
     */
    class HeightArrayState : public vsgUtil::ArrayState<HeightArrayState>
    {
    public:
        float scale{};
        int numVerts{};
        using vsg::ConstVisitor::apply;
        void apply(const vsg::floatArray& in_heights) override
        {
            // vsgopenmw-array-state-reuse-vertices
            proxy_vertices = vsg::vec3Array::create(in_heights.size()); // = availableVertexCache.get(minCount=in_verts.size());
            for (size_t i = 0; i < in_heights.size(); ++i)
            {
                int vertX = i / numVerts;
                int vertY = i % numVerts;
                proxy_vertices->at(i) = {
                    (vertX / float(numVerts-1) - 0.5f) * scale,
                    (vertY / float(numVerts-1) - 0.5f) * scale,
                    in_heights.at(i) };
            }
            vertices = proxy_vertices;
        }
    };

    Grid::Grid(
        Storage* storage, vsg::ref_ptr<vsgUtil::CompileContext> compile, vsg::ref_ptr<const vsg::Options> options, vsg::ref_ptr<const vsg::Options> shaderOptions, vsg::ref_ptr<vsg::Sampler> samplerOptions)
        : Group(storage, options, shaderOptions)
        , mLayers(options, shaderOptions, samplerOptions)
        , mCompile(compile)
    {
        mNode = vsg::Group::create();
    }

    Grid::~Grid() {}

    vsg::ref_ptr<vsg::Node> Grid::buildTerrain(float chunkSize, const vsg::vec2& chunkCenter) const
    {
        if (chunkSize * mNumSplits > 1.f)
        {
            float newChunkSize = chunkSize / 2.f;
            vsg::Group::Children children
                = { buildTerrain(newChunkSize, chunkCenter + vsg::vec2(newChunkSize / 2.f, newChunkSize / 2.f)),
                      buildTerrain(newChunkSize, chunkCenter + vsg::vec2(newChunkSize / 2.f, -newChunkSize / 2.f)),
                      buildTerrain(newChunkSize, chunkCenter + vsg::vec2(-newChunkSize / 2.f, newChunkSize / 2.f)),
                      buildTerrain(newChunkSize, chunkCenter + vsg::vec2(-newChunkSize / 2.f, -newChunkSize / 2.f)) };
            return vsgUtil::createCullNode(children);
        }
        else
        {
            auto sg = mLayers.create(*mStorage, chunkSize, chunkCenter);
            sg->addChild(createGeometry(*mStorage, chunkSize, chunkCenter, 0, 0));
            auto arrayState = vsg::ref_ptr{ new HeightArrayState };
            arrayState->scale = chunkSize * mStorage->cellWorldSize;
            arrayState->numVerts = (mStorage->cellVertices-1)*chunkSize+1;
            sg->prototypeArrayState = arrayState;
            vsg::dvec3 center (chunkCenter.x * mStorage->cellWorldSize, chunkCenter.y * mStorage->cellWorldSize, 0);
            auto transform = vsg::MatrixTransform::create();
            transform->subgraphRequiresLocalFrustum = false;
            transform->matrix[3] = vsg::dvec4(center, 1.0);
            transform->children = { sg };

            float minH, maxH;
            mStorage->getMinMaxHeights(chunkSize, chunkCenter, minH, maxH);
            double radius = chunkSize * mStorage->cellWorldSize / 2;
            vsg::dbox bb(vsg::dvec3(center.x - radius, center.y - radius, minH), vsg::dvec3(center.x + radius, center.y + radius, maxH));
            return vsg::CullNode::create(vsgUtil::toSphere(bb), transform);
        }
    }

    void Grid::loadCell(int x, int y)
    {
        auto coord = std::make_pair(x, y);
        if (mGrid.find(coord) != mGrid.end())
            return;

        if (auto terrainNode = mCache.getOrCreate(coord, *this))
        {
            if (!mCompile->compile(terrainNode))
                return;
            mNode->addChild(terrainNode);
            mGrid[std::make_pair(x, y)] = terrainNode;
        }
    }

    vsg::ref_ptr<vsgUtil::Operation> Grid::cacheCell(int x, int y)
    {
        auto coord = std::make_pair(x, y);
        if (auto node = mCache.getOrCreate(coord, *this))
            return vsg::ref_ptr{ new vsgUtil::CompileOp(node, mCompile) };
        return {};
    }

    vsg::ref_ptr<vsg::Node> Grid::create(const Coord& coord) const
    {
        vsg::vec2 center(coord.first + 0.5f, coord.second + 0.5f);
        return buildTerrain(1.f, center);
    }

    void Grid::unloadCell(int x, int y)
    {
        auto it = mGrid.find(std::make_pair(x, y));
        if (it == mGrid.end())
            return;

        mCompile->detach(it->second);
        vsgUtil::removeChild(mNode, it->second);
        mGrid.erase(it);
        mCache.prune();
    }
}

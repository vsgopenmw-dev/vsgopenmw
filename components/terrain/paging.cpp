#include "paging.hpp"

#include <components/misc/mathutil.hpp>

#include "storage.hpp"
#include "lod.hpp"
#include "viewdata.hpp"
#include "builder.hpp"

namespace Terrain
{
    Paging::Paging(vsg::ref_ptr<vsgUtil::CompileContext> compile, vsg::ref_ptr<const vsg::Options> imageOptions, vsg::ref_ptr<const vsg::Options> shaderOptions, vsg::ref_ptr<vsg::Sampler> samplerOptions)
        : mCompile(compile)
    {
        mBuilder = std::make_unique<Builder>(imageOptions, shaderOptions, samplerOptions);
    }

    Paging::~Paging() {}

    std::unique_ptr<View> Paging::createView() const
    {
        return std::make_unique<ViewData>();
    }

    int Paging::getBatchSize(float cellViewDist) const
    {
        int batchSize = std::max(1, Misc::nextPowerOfTwo(std::ceil(cellViewDist * 2 / 4)));
        return std::min(mMaxBatchSize, batchSize);
    }

    vsg::ref_ptr<vsgUtil::CompileOp> Paging::updateViewForCell(View& v, const vsg::ivec2& cell, float referenceViewDistance) const
    {
        ViewData& view = static_cast<ViewData&>(v);

        int batchSize = getBatchSize(referenceViewDistance / mStorage->cellWorldSize);
        int x = std::floor(cell.x / batchSize);
        int y = std::floor(cell.y / batchSize);
        Bounds batchBounds { static_cast<float>(batchSize), vsg::vec2(x * batchSize, y * batchSize) };
        auto batch = mBuilder->createBatch(batchBounds, 0);
        Bounds drawBounds { 1.f, vsg::vec2(cell) };
        auto draw = mBuilder->createDraw(batch, drawBounds, 0, 0);
        batch.leafGroup->addChild(draw);

        for (auto& child : view.rootNode->children)
            mCompile->detach(child);
        view.rootNode->children = { batch.root };
        return vsg::ref_ptr{ new vsgUtil::CompileOp(view.rootNode, mCompile, [&view]() { view.clear(); }) };
    }

    vsg::ref_ptr<vsgUtil::CompileOp> Paging::updateView(View& v, const vsg::dvec3& viewPoint, float viewDistance, bool preload) const
    {
        ViewData& view = static_cast<ViewData&>(v);

        if (vsg::length2(view.viewPoint - viewPoint) > View::sqrReuseDistance || view.viewDistance != viewDistance)
        {
            view.reset();
            view.viewPoint = viewPoint;
            view.viewDistance = viewDistance;

            const double cellWorldSize = mStorage->cellWorldSize;
            vsg::dvec2 viewCell = vsg::dvec2(viewPoint.x, viewPoint.y) / cellWorldSize;
            double cellViewDistance = viewDistance / cellWorldSize;
            int batchSize = getBatchSize(cellViewDistance);

            double cellViewDistanceLoad = cellViewDistance + View::reuseDistance / mStorage->cellWorldSize;
            if (preload)
                cellViewDistanceLoad += 1.0;

            int minX = std::floor((viewCell.x-cellViewDistanceLoad)/batchSize);
            int maxX = std::ceil((viewCell.x+cellViewDistanceLoad)/batchSize);
            int minY = std::floor((viewCell.y-cellViewDistanceLoad)/batchSize);
            int maxY = std::ceil((viewCell.y+cellViewDistanceLoad)/batchSize);

            std::function<void(const Bounds& bounds)> traverse = [this, &view, viewCell, cellViewDistanceLoad, &traverse](const Bounds& bounds) {
                float cellDist = distance(bounds, viewCell);
                if (cellDist > cellViewDistanceLoad)
                    return;
                if (isSufficientDetail(bounds.size, cellDist, lodFactor))
                {
                    view.add(bounds, cellDist);
                }
                else
                {
                    float childSize = bounds.radius();
                    traverse({ childSize, bounds.min });
                    traverse({ childSize, bounds.min + vsg::vec2(childSize, childSize) });
                    traverse({ childSize, bounds.min + vsg::vec2(0.f, childSize) });
                    traverse({ childSize, bounds.min + vsg::vec2(childSize, 0.f) });
                }
            };

            struct Batch
            {
                size_t start;
                size_t end;
                float distance;
                Bounds bounds;
            };
            std::vector<Batch> batches;

            for (int x = minX; x < maxX; ++x)
            {
                for (int y = minY; y < maxY; ++y)
                {
                    Bounds bounds { static_cast<float>(batchSize), vsg::vec2(x * batchSize, y * batchSize) };

                    auto start = view.numEntries;
                    traverse(bounds);
                    auto end = view.numEntries;
                    if (start != end)
                    {
                        float dist = distance(bounds, viewCell);
                        batches.push_back(Batch{ start, end, dist, bounds });
                    }
                }
            }

            if (view.changed)
            {
                //drawFrontToBack()
                std::sort(batches.begin(), batches.end(), [](const Batch& lhs, const Batch& rhs) -> auto { return lhs.distance < rhs.distance; });

                auto getLodFlags = [this, &view](const Bounds& bounds, uint32_t ourVertexLod) -> uint32_t
                {
                    uint32_t lodFlags = 0;
                    vsg::vec2 directions[4] = {
                        { 0.f, 1.f }, // North
                        { 1.f, 0.f }, // East
                        { 0.f, -1.f }, // South
                        { -1.f, 0.f } // West
                    };
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        auto neighbourCenter = bounds.center() + directions[i] * bounds.size;
                        auto neighbour = view.getContainingChunk(neighbourCenter);
                        // We only need to worry about neighbours less detailed than we are - neighbours with more detail will do the stitching themselves
                        if (!neighbour.valid() || neighbour.size <= bounds.size)
                            continue;
                        uint32_t lod = getVertexLod(neighbour.size, vertexLodMod);
                        if (lod > ourVertexLod)
                            lodFlags |= (lod - ourVertexLod) << (4 * i);
                    }
                    return lodFlags;
                };

                for (auto& oldChild : view.rootNode->children)
                    mCompile->detach(oldChild);
                view.rootNode->children.clear();
                view.rootNode->children.reserve(batches.size());

                for (const auto& viewBatch : batches)
                {
                    // At the moment, applying lod to batches won't help because we have to loop over all affected cells' data anyway.
                    auto batch = mBuilder->createBatch(viewBatch.bounds, /*lod*/ 0);

                    std::vector<size_t> indices;
                    indices.reserve(viewBatch.end-viewBatch.start);
                    for (size_t i = viewBatch.start; i < viewBatch.end; ++i)
                        indices.push_back(i);
                    // drawFrontToBack
                    std::sort(indices.begin(), indices.end(), [view](size_t lhs, size_t rhs) -> auto { return view.entries[lhs].distance < view.entries[rhs].distance; });

                    for (auto i : indices)
                    {
                        const auto& entry = view.entries[i];
                        auto vertexLod = getVertexLod(entry.bounds.size, vertexLodMod);
                        auto lodFlags = getLodFlags(entry.bounds, vertexLod);
                        auto node = mBuilder->createDraw(batch, entry.bounds, vertexLod, lodFlags);
                        batch.leafGroup->addChild(node);
                    }
                    view.rootNode->children.push_back(batch.root);
                }
                return vsg::ref_ptr{ new vsgUtil::CompileOp(view.rootNode, mCompile, [&view]() { view.clear(); }) };
            }
        }
        return {};
    }

    void Paging::clearView(View& v) const
    {
        ViewData& view = static_cast<ViewData&>(v);
        for (auto& oldChild : view.rootNode->children)
            mCompile->detach(oldChild);
        view.clear();
    }

    void Paging::pruneCache() const
    {
        mBuilder->pruneCache();
    }

    void Paging::setStorage(Storage* storage)
    {
        mStorage = storage;
        mBuilder->setStorage(storage);
    }
}

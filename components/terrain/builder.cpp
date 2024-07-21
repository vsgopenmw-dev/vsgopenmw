#include "builder.hpp"

#include <vsg/utils/SharedObjects.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/DescriptorBuffer.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/commands/DrawIndexed.h>
#include <vsg/commands/Commands.h>

#include <components/pipeline/sets.hpp>
#include <components/pipeline/terrain.hpp>
#include <components/vsgutil/arraystate.hpp>
#include <components/vsgutil/bounds.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/name.hpp>
#include <components/vsgutil/arraytexture.hpp>

#include "indexbuffer.hpp"

namespace Pipeline::Descriptors
{
    #include <files/shaders/terrain/bindings.glsl>
}
namespace Pipeline::Data
{
    using vsg::vec2;
    #include <files/shaders/terrain/data.glsl>
}
namespace Terrain
{
    /*
     * Reproduces vertex shader for intersection testing on the CPU.
     */
    class HeightArrayState : public vsgUtil::ArrayState<HeightArrayState>
    {
        const vsg::floatArray2D* mHeights = nullptr;
    public:
        vsg::Object* parent{};
        float vertexScale = 0;
        vsg::vec2 origin;
        using vsg::ConstVisitor::apply;

        void apply(const vsg::BindDescriptorSet& bds) override
        {
            apply(*bds.descriptorSet);
        }
        void apply(const vsg::BindVertexBuffers&) override {}
        void apply(const vsg::DescriptorSet& ds) override
        {
            for (auto& descriptor : ds.descriptors)
            {
                if (descriptor->dstBinding == Pipeline::Descriptors::HEIGHTMAP_BINDING)
                {
                    descriptor->accept(*this);
                    break;
                }
            }
        }
        void apply(const vsg::DescriptorImage& di) override
        {
            di.imageInfoList[0]->imageView->image->data->accept(*this);
        }
        void apply(const vsg::floatArray2D& in_heights) override
        {
            mHeights = &in_heights;
        }
        vsg::ref_ptr<const vsg::vec3Array> vertexArray(uint32_t instance) override
        {
            uint8_t lod = instance & 0xff;
            uint32_t shiftSize = (instance >> 8) & 0xff;
            float chunkScale = 1.f / (1 << shiftSize);
            vsg::ivec2 gridPos = vsg::ivec2((instance >> 16) & 0xff, (instance >> 24) & 0xff);
            vsg::vec2 chunkOffset = vsg::vec2(gridPos) * chunkScale;

            auto w = mHeights->width();
            auto h = mHeights->height();
            uint32_t increment = 1 << lod;
            auto chunkW = (w-1) * chunkScale/ increment + 1;
            auto chunkH = (h-1) * chunkScale/ increment + 1;
            vsg::ref_ptr<vsg::vec3Array> proxy (getOrCreateProxyVertices(parent, chunkW * chunkH));
            uint32_t x0 = (w-1) * chunkOffset.x;
            uint32_t y0 = (h-1) * chunkOffset.y;
            for (uint32_t x = 0; x < chunkW; ++x)
            {
                for (uint32_t y = 0; y < chunkH; ++y)
                {
                    auto srcX = x0 + x * increment;
                    auto srcY = y0 + y * increment;
                    proxy->at(x * chunkH + y) = {
                        srcX / float(w-1) * vertexScale + origin.x,
                        srcY / float(h-1) * vertexScale + origin.y,
                        mHeights->at(srcX, srcY) };
                }
            }
            return proxy;
        }
    };

    Builder::Builder(vsg::ref_ptr<const vsg::Options> in_imageOptions, vsg::ref_ptr<const vsg::Options> in_shaderOptions, vsg::ref_ptr<vsg::Sampler> in_samplerOptions)
        : mImageOptions(in_imageOptions)
        , mSamplerOptions(in_samplerOptions)
    {
        mPipeline = Pipeline::terrain(in_shaderOptions);
    }

    Builder::~Builder() {}

    void Builder::pruneCache() const
    {
        dataCache.removeObjects([](const Builder::BatchData& d) -> bool {
            for (auto& sc : d.stateCommands)
            {
                if (sc->referenceCount() <= 1)
                    return true;
            }
            return false;
        });
    }

    void Builder::setStorage(Storage* in_storage)
    {
        mStorage = in_storage;

        auto layers = mStorage->getLayers();
        if (layers.size() > 256)
        {
            // Vulkan guarantees a minimum of 256 maxImageArrayLayers.
            // uint8_t type is used for blendMap channels.
            std::cerr << "layerInfos.size() > 256" << std::endl;
            layers.resize(256);
        }

        vsg::DataList imageData;
        for (const auto& layer : layers)
        {
            auto data = vsgUtil::readImage(layer, mImageOptions);
            vsgUtil::setName(*data, layer);
            imageData.push_back(data);
        }
        auto arrayData = vsgUtil::convertToArrayTexture(imageData);
        mLayerDescriptor = vsg::DescriptorImage::create(mSamplerOptions, arrayData, Pipeline::Descriptors::LAYER_BINDING);
    }

    Builder::BatchData Builder::create(const Key& key) const
    {
        const auto& bounds = key.first;
        auto lod = key.second;

        auto data = mStorage->getVertexData(lod, bounds);

        vsg::ref_ptr<vsg::Data> blendmap = mStorage->getBlendmap(bounds);

        auto layout = mPipeline->pipeline->layout;

        auto blendSampler = vsg::Sampler::create();
        if (auto& sharedObjects = mImageOptions->sharedObjects)
            sharedObjects->share(blendSampler);
        auto blendDescriptor = vsg::DescriptorImage::create(blendSampler, blendmap, Pipeline::Descriptors::BLENDMAP_BINDING);

        auto heightsDescriptor = vsg::DescriptorImage::create(mSamplerOptions, data.heights, Pipeline::Descriptors::HEIGHTMAP_BINDING);
        auto normalsDescriptor = vsg::DescriptorImage::create(mSamplerOptions, data.normals, Pipeline::Descriptors::NORMALMAP_BINDING);
        auto colorDescriptor = vsg::DescriptorImage::create(mSamplerOptions, data.colors, Pipeline::Descriptors::COLORMAP_BINDING);

        auto batchData = vsg::Value<Pipeline::Data::Batch>::create(Pipeline::Data::Batch{
            .origin = bounds.min,
            .numVerts = static_cast<float>(mStorage->numVerts(bounds.size, lod)),
            .vertexScale = mStorage->cellWorldSize * bounds.size
        });
        auto batchDescriptor = vsg::DescriptorBuffer::create(batchData, Pipeline::Descriptors::BATCH_DATA_BINDING);

        vsg::Descriptors descriptors = { blendDescriptor, batchDescriptor, mLayerDescriptor, heightsDescriptor, normalsDescriptor, colorDescriptor };

        auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::TEXTURE_SET, descriptors);

        /*
         * Assigns grid of height ranges so we can quickly look up the bounds for a given chunk.
         */
        uint32_t gridSize = bounds.size/Bounds::minSize;
        auto ranges = vsg::vec2Array2D::create(gridSize, gridSize);
        {
            for (uint32_t gridX = 0; gridX < gridSize; ++gridX)
            {
                for (uint32_t gridY = 0; gridY < gridSize; ++gridY)
                {
                    float minH = std::numeric_limits<float>::max();
                    float maxH = -std::numeric_limits<float>::max();
                    uint32_t verts0 = mStorage->numVerts(bounds.size / gridSize, lod);
                    uint32_t startX = gridX * (verts0-1);
                    uint32_t startY = gridY * (verts0-1);
                    for (uint32_t x = startX; x < startX + verts0; ++x)
                    {
                        for (uint32_t y = startY; y < startY + verts0; ++y)
                        {
                            float h = data.heights->at(x, y);
                            maxH = std::max(maxH, h);
                            minH = std::min(minH, h);
                        }
                    }
                    ranges->at(gridX, gridY) = { minH, maxH };
                }
            }
        }

        return { vsg::StateCommands{ mPipeline, bds }, ranges };
    }

    Builder::Batch Builder::createBatch(const Bounds& bounds, uint8_t lod) const
    {
        auto batchData = getBatchData(bounds, lod);
        auto sg = vsg::StateGroup::create();
        sg->stateCommands = batchData.stateCommands;

        vsg::vec2 origin = bounds.min * mStorage->cellWorldSize;

        auto arrayState = HeightArrayState::create();
        arrayState->vertexScale = bounds.size * mStorage->cellWorldSize;
        arrayState->origin = origin;
        sg->prototypeArrayState = arrayState;
        arrayState->parent = sg.get();

        auto cullNode = createCullNode(sg, bounds, batchData, bounds);
        return { bounds, lod, batchData, cullNode, sg };
    }

    Builder::BatchData Builder::getBatchData(const Bounds& bounds, uint8_t lod) const
    {
        return dataCache.getOrCreate(std::make_pair(bounds, lod), *this);
    }

    vsg::ref_ptr<vsg::Node> Builder::createDraw(const Builder::Batch& batch, const Bounds& bounds, uint8_t lod, uint32_t lodFlags) const
    {
        uint32_t numVerts = mStorage->numVerts(bounds.size, lod);
        auto bindIndexBuffer = indexCache.getOrCreate(std::make_pair(numVerts, lodFlags), CreateIndexBuffer());
        uint32_t instance = static_cast<uint32_t>(lod - batch.lod);
        uint32_t shiftSize = ilog2(batch.bounds.size/bounds.size);
        instance |= shiftSize << 8;
        vsg::vec2 gridPos = (bounds.min - batch.bounds.min) / bounds.size;
        instance |= static_cast<uint32_t>(gridPos.x) << 16;
        instance |= static_cast<uint32_t>(gridPos.y) << 24;
        auto draw = vsg::DrawIndexed::create(bindIndexBuffer->indices->data->valueCount(), 1, 0, 0, instance);
        auto commands = vsg::Commands::create();
        commands->children = { bindIndexBuffer, draw };
        if (bounds.size < batch.bounds.size)
            return createCullNode(commands, batch.bounds, batch.data, bounds);
        else
            return commands;
    }

    vsg::ref_ptr<vsg::Node> Builder::createCullNode(vsg::ref_ptr<vsg::Node> child, const Bounds& batchBounds, const Builder::BatchData& data, const Bounds& bounds) const
    {
        vsg::vec2 heightRangeGridPos = (bounds.min - batchBounds.min) / Bounds::minSize;
        uint32_t heightRangeWidth = static_cast<uint32_t>(bounds.size / Bounds::minSize);
        float minH = std::numeric_limits<float>::max();
        float maxH = -std::numeric_limits<float>::max();
        for (uint32_t x = 0; x < heightRangeWidth; ++x)
        {
            for (uint32_t y = 0; y < heightRangeWidth; ++y)
            {
                uint32_t hx = x + heightRangeGridPos.x;
                uint32_t hy = y + heightRangeGridPos.y;
                hx = std::max(0u, std::min(hx, data.heightRanges->width()-1));
                hy = std::max(0u, std::min(hy, data.heightRanges->height()-1));
                const auto& range = data.heightRanges->at(hx, hy);
                minH = std::min(range.x, minH);
                maxH = std::max(range.y, maxH);
            }
        }
        float worldSize = bounds.size * mStorage->cellWorldSize;
        vsg::vec2 offset = bounds.min * mStorage->cellWorldSize;
        vsg::dbox bb = { vsg::dvec3(offset.x, offset.y, minH), vsg::dvec3(offset.x + worldSize, offset.y + worldSize, maxH) };
        return vsg::CullNode::create(vsgUtil::toSphere(bb), child);
    }
}

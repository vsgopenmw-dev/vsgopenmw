#include "map.hpp"

#include <vsg/app/View.h>
#include <vsg/commands/BlitImage.h>
#include <vsg/commands/Commands.h>
#include <vsg/commands/PipelineBarrier.h>
#include <vsg/io/Options.h>
#include <vsg/maths/vec2.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/PushConstants.h>
#include <vsg/vk/Context.h>
#include <vsg/utils/ComputeBounds.h>

#include <components/esm3/fogstate.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/misc/constants.hpp>
#include <components/pipeline/fogofwar.hpp>
#include <components/pipeline/sets.hpp>
#include <components/render/computeimage.hpp>
#include <components/render/rendertexture.hpp>
#include <components/render/attachmentformat.hpp>
#include <components/view/collectlights.hpp>
#include <components/view/defaultstate.hpp>
#include <components/view/scene.hpp>
#include <components/view/descriptors.hpp>
#include <components/vsgutil/imageio.hpp>
#include <components/vsgutil/sharedview.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/bounds.hpp>
#include <components/vsgutil/image.hpp>
#include <components/vsgutil/nullbin.hpp>
#include <components/terrain/paging.hpp>

#include "../mwworld/cellstore.hpp"

#include "mask.hpp"
#include "bin.hpp"

namespace FogConstants
{
#include <files/shaders/comp/fogofwar/constants.glsl>
}
namespace
{
    auto sFogTextureFormat = VK_FORMAT_R8_UNORM;

    vsg::vec2 getNorthVector(const MWWorld::CellStore* cell)
    {
        MWWorld::ConstPtr northmarker = cell->searchConst(ESM::RefId::stringRefId("northmarker"));

        if (northmarker.isEmpty())
            return {0, 1};

        vsg::quat orient(-northmarker.getRefData().getPosition().rot[2], vsg::vec3(0, 0, 1));
        auto dir = orient * vsg::vec3(0, 1, 0);
        return {dir.x, dir.y};
    }

    vsg::dvec2 rotatePoint(const vsg::dvec2& point, const vsg::dvec2& center, float angle)
    {
        return { std::cos(angle) * (point.x - center.x) - std::sin(angle) * (point.y - center.y) + center.x,
            std::sin(angle) * (point.x - center.x) + std::cos(angle) * (point.y - center.y) + center.y };
    }

    std::pair<int, int> divideIntoSegments(const vsg::dbox& bounds, float mapSize)
    {
        auto length = vsgUtil::extent(bounds);
        return { static_cast<int>(std::ceil(length.x / mapSize)), static_cast<int>(std::ceil(length.y / mapSize)) };
    }
    float square(float v)
    {
        return v * v;
    }
}

namespace MWRender
{
    struct Map::ViewData
    {
        std::unique_ptr<View::Scene> sceneData;
        vsg::ref_ptr<vsg::DescriptorSet> descriptorSet;
        vsg::ref_ptr<vsg::StateGroup> state;
        std::unique_ptr<Terrain::View> terrainView;
        vsg::Group::Children sceneChildren;
    };

    class Map::Blit : public vsg::Group
    {
        vsg::ref_ptr<vsg::BlitImage> mBlit;
        vsg::ref_ptr<vsg::PipelineBarrier> mPreBarrier;
        vsg::ref_ptr<vsg::PipelineBarrier> mPostBarrier;
        vsg::ref_ptr<vsg::View> mView;
        vsg::ref_ptr<vsg::Group> mSceneRoot;

    public:

        Blit(int32_t resolution, vsg::ref_ptr<vsg::Image> srcImage, vsg::ref_ptr<vsg::View> view, vsg::ref_ptr<vsg::Group> sceneRoot)
            : mView(view)
            , mSceneRoot(sceneRoot)
        {
            VkImageBlit region{};
            region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
            region.srcOffsets[0] = region.dstOffsets[0] = VkOffset3D{ 0, 0, 0 };
            region.srcOffsets[1] = region.dstOffsets[1] = VkOffset3D{ resolution, resolution, 1 };

            mBlit = vsg::BlitImage::create();
            mBlit->srcImage = srcImage;
            mBlit->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            mBlit->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            mBlit->regions = { region };
            mBlit->filter = VK_FILTER_NEAREST;

            VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            mPreBarrier = vsg::PipelineBarrier::create(
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0 /*VK_DEPENDENCY_BY_REGION_BIT*/,
                vsg::MemoryBarrier::create(
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT), // srcImage
                vsg::ImageMemoryBarrier::create(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    /*dstImage*/ vsg::ref_ptr<vsg::Image>(), range));

            mPostBarrier = vsg::PipelineBarrier::create(VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT/*supportsWorldMap*/,
                0 /*VK_DEPENDENCY_BY_REGION_BIT*/,
                vsg::MemoryBarrier::create(
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT), // srcImage
                vsg::ImageMemoryBarrier::create(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                    /*dstImage*/ vsg::ref_ptr<vsg::Image>(), range));
        }
        struct Request
        {
            vsg::ref_ptr<vsg::ViewMatrix> viewMatrix;
            vsg::ref_ptr<vsg::Orthographic> projectionMatrix;
            vsg::ref_ptr<vsg::Image> dstImage;
            Map::ViewData* viewData{};
        };
        mutable std::vector<Request> requests;
        mutable std::vector<Map::ViewData*> availableViewData;

        void accept(vsg::RecordTraversal& visitor) const override
        {
            if (requests.empty() || !(visitor.traversalMask & mView->mask))
                return;
            for (auto& r : requests)
            {
                mView->camera->projectionMatrix = r.projectionMatrix;
                mView->camera->viewMatrix = r.viewMatrix;
                mSceneRoot->children = r.viewData->sceneChildren;

                for (auto& sc : r.viewData->state->stateCommands)
                    sc->accept(visitor);

                traverse(visitor);

                mPreBarrier->imageMemoryBarriers[0]->image = r.dstImage;
                visitor.apply(*mPreBarrier);

                mBlit->dstImage = r.dstImage;
                visitor.apply(*mBlit);

                mPostBarrier->imageMemoryBarriers[0]->image = r.dstImage;
                visitor.apply(*mPostBarrier);
                availableViewData.push_back(r.viewData);
            }
            requests.clear();
        }
    };

    class Map::UpdateFog : public Render::ComputeImage
    {
        vsg::ref_ptr<vsg::vec4Array> mData = vsg::vec4Array::create(2);

    public:
        vsg::PipelineLayout* const layout;
        UpdateFog(vsg::ref_ptr<vsg::BindComputePipeline> pipeline)
            : layout(pipeline->pipeline->layout)
        {
            auto pc = vsg::PushConstants::create(VK_SHADER_STAGE_COMPUTE_BIT, 0, mData);
            mDispatch = vsg::Dispatch::create(0, 0, 1);
            auto commands = vsg::Commands::create();
            commands->children = { pipeline, pc, mDispatch };
            children = { commands };
            mClearColorImage->color = { { 1, 1, 1, 1 } };
        }
        struct Request
        {
            vsg::ref_ptr<vsg::BindDescriptorSet> bds;
            vsg::ref_ptr<vsg::Image> image;
            vsg::vec4 rect;
            vsg::vec2 center;
        };
        mutable std::vector<Request> requests;
        void accept(vsg::RecordTraversal& visitor) const override
        {
            if (requests.empty() && clearRequests.empty())
                return;

            handleClearRequests(visitor);

            for (auto& r : requests)
            {
                mData->at(0) = r.rect;
                mData->at(1) = vsg::vec4(r.center.x, r.center.y, 0, 0 );
                visitor.apply(*r.bds);

                #include <files/shaders/comp/fogofwar/workgroupsize.glsl>
                // Vulkan guarantees a minimum of 128 maxComputeWorkGroupInvocations.
                static_assert(workGroupSize * workGroupSize <= 128);
                compute(visitor, r.image, r.rect.z - r.rect.x, r.rect.w - r.rect.y, workGroupSize);
            }
            requests.clear();
        }
    };

    struct Map::Texture
    {
        vsg::ref_ptr<vsg::ImageInfo> imageInfo;
    };
    struct Map::FogTexture : public Map::Texture
    {
        vsg::ref_ptr<vsg::BindDescriptorSet> bds;
        vsg::ref_ptr<vsg::ubyteArray2D> data;
    };

    Map::Map(vsg::Context& ctx, VkSampleCountFlagBits samples, vsg::ref_ptr<vsgUtil::CompileContext> compile, vsg::ref_ptr<vsg::Node> scene, Terrain::Paging* terrain, float referenceViewDistance, uint32_t resolution, const vsg::Options* shaderOptions)
        : mContext(&ctx)
        , mSceneGraph(scene)
        , mTerrain(terrain)
        , mReferenceViewDistance(referenceViewDistance)
        , mMapWorldSize(Constants::CellSizeInUnits) // ESM3::Cell::Size
        , mCellDistance(1)
        , mResolution(resolution)
        , mCompile(compile)
    {
        auto camera = vsg::Camera::create();
        camera->viewportState = vsg::ViewportState::create(0, 0, resolution, resolution);

        auto collectLights = vsg::ref_ptr{ new View::CollectLights(256) }; //CollectAndCullLights(64)
        mLightData = collectLights->data();

        auto root = vsg::Group::create();
        root->children = { mSceneGraph }; // compileSimpleWater()
        auto view = vsgUtil::createSharedView(camera, root);
        // auto lightGrid = std::make_unique<View::LightGrid>(shaderOptions);
        // setResolution()
        view->mask = ~(Mask_Particle|Mask_GUI);
        view->viewDependentState = collectLights;
        view->bins = {
            vsg::Bin::create(Bin_DepthSorted, vsg::Bin::DESCENDING),
            vsgUtil::NullBin::create(Bin_Compute)
        };

        auto [renderGraph, srcImageView] = Render::createRenderTexture(ctx, samples, VkExtent2D{ resolution, resolution }, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL); // vsgopenmw-optimization-mwrender-map-reuse-render-texture
        renderGraph->setClearValues(VkClearColorValue{ { 0, 0, 0, 1 } });
        renderGraph->children = { view };

        mBlit = new Blit(resolution, srcImageView->image, view, root);
        mBlit->children = { renderGraph };

        auto fogPipeline = Pipeline::fogOfWar(vsg::ref_ptr{ shaderOptions });
        mUpdateFog = new UpdateFog(fogPipeline);
    }

    Map::~Map() {}

    void Map::clear()
    {
        clearSegments();
        mBlit->requests.clear();
    }

    void Map::recycle(Segment& segment)
    {
        if (segment.mapTexture)
            mAvailableTextures.push_back(segment.mapTexture);
        if (segment.fogTexture)
            mAvailableFogTextures.push_back(segment.fogTexture);
    }

    void Map::clearSegments()
    {
        for (auto& [coord, segment] : mSegments)
            recycle(segment);
        mSegments.clear();
    }

    void Map::saveFogOfWar(MWWorld::CellStore* cell)
    {
        if (!mInterior)
        {
            auto found = mSegments.find(std::make_pair(cell->getCell()->getGridX(), cell->getCell()->getGridY()));
            if (found == mSegments.end())
                return;
            const Segment& segment = found->second;
            if (segment.fogTexture && segment.hasFogState)
            {
                auto fog = std::make_unique<ESM::FogState>();
                fog->mFogTextures.emplace_back();
                segment.saveFogOfWar(fog->mFogTextures.back());
                cell->setFog(std::move(fog));
            }
        }
        else
        {
            auto segments = divideIntoSegments(mBounds, mMapWorldSize);
            auto fog = std::make_unique<ESM::FogState>();
            fog->mBounds.mMinX = mBounds.min.x;
            fog->mBounds.mMaxX = mBounds.max.x;
            fog->mBounds.mMinY = mBounds.min.y;
            fog->mBounds.mMaxY = mBounds.max.y;
            fog->mNorthMarkerAngle = mAngle;
            fog->mFogTextures.reserve(segments.first * segments.second);
            for (int x = 0; x < segments.first; ++x)
            {
                for (int y = 0; y < segments.second; ++y)
                {
                    const Segment& segment = mSegments[std::make_pair(x, y)];

                    fog->mFogTextures.emplace_back();

                    if (segment.hasFogState)
                        segment.saveFogOfWar(fog->mFogTextures.back());

                    fog->mFogTextures.back().mX = x;
                    fog->mFogTextures.back().mY = y;
                }
            }
            cell->setFog(std::move(fog));
        }
    }

    Map::ViewData* Map::setupRenderToTexture(
        int segment_x, int segment_y, float x, float y, const vsg::dvec3& upVector, float zmin, float zmax)
    {
        auto tex = getOrCreateTexture();
        if (!tex)
            return {};

        auto request = Blit::Request();
        request.projectionMatrix = vsg::Orthographic::create(
            -mMapWorldSize / 2, mMapWorldSize / 2, -mMapWorldSize / 2, mMapWorldSize / 2, 5, (zmax - zmin) + 10);
        request.viewMatrix = vsg::LookAt::create(vsg::dvec3(x, y, zmax + 5), vsg::dvec3(x, y, zmin), upVector);
        request.dstImage = tex->imageInfo->imageView->image;
        auto viewData = getOrCreateViewData();
        if (!viewData)
            return {};
        viewData->sceneData->value().invView = vsg::mat4(vsg::inverse_4x3(request.viewMatrix->transform()));
        viewData->sceneData->setLightPosition({ -0.3, -0.3, 0.7 }, viewData->sceneData->value().invView);
        viewData->sceneData->setDepthRange(
            request.projectionMatrix->nearDistance, request.projectionMatrix->farDistance);
        request.viewData = viewData;

        mBlit->requests.push_back(request);

        mSegments[std::make_pair(segment_x, segment_y)].mapTexture = tex;
        return viewData;
    }

    Map::Texture* Map::getOrCreateTexture()
    {
        if (!mAvailableTextures.empty())
        {
            auto tex = mAvailableTextures.back();
            mAvailableTextures.pop_back();
            return tex;
        }

        auto dstImage = vsgUtil::createImage(Render::compatibleColorFormat, VkExtent2D{ mResolution, mResolution }, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        auto imageInfo = vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(),
            vsg::createImageView(*mContext, dstImage, VK_IMAGE_ASPECT_COLOR_BIT), VK_IMAGE_LAYOUT_UNDEFINED);
        auto tex = std::make_unique<Texture>();
        tex->imageInfo = imageInfo;
        mTextures.emplace_back(std::move(tex));
        return mTextures.back().get();
    }

    Map::FogTexture* Map::getOrCreateFogTexture()
    {
        if (!mAvailableFogTextures.empty())
        {
            auto tex = mAvailableFogTextures.back();
            mAvailableFogTextures.pop_back();
            return tex;
        }

        auto dstImage = vsgUtil::createImage(sFogTextureFormat, VkExtent2D{ FogConstants::resolution, FogConstants::resolution }, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT);

        auto imageView = vsg::ImageView::create(dstImage, VK_IMAGE_ASPECT_COLOR_BIT);
        imageView->components
            = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
        auto storageImageView = vsg::ImageView::create(
            dstImage, VK_IMAGE_ASPECT_COLOR_BIT); // VUID-VkWriteDescriptorSet-descriptorType-00336
        auto storageImage = vsg::DescriptorImage::create(
            vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), storageImageView, VK_IMAGE_LAYOUT_GENERAL), 0, 0,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, mUpdateFog->layout,
            Pipeline::COMPUTE_SET, vsg::Descriptors{ storageImage });
        if (!mCompile->compile(bds))
            return {};

        auto tex = std::make_unique<FogTexture>();
        tex->bds = bds;
        tex->data = vsg::ubyteArray2D::create(
            FogConstants::resolution, FogConstants::resolution, 0xff, vsg::Data::Properties{ sFogTextureFormat });
        dstImage->data = tex->data;
        tex->data->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;
        tex->imageInfo
            = vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        mFogTextures.emplace_back(std::move(tex));
        return mFogTextures.back().get();
    }

    Map::ViewData* Map::getOrCreateViewData()
    {
        auto& available = mBlit->availableViewData;
        if (!available.empty())
        {
            auto d = available.back();
            available.pop_back();
            return d;
        }
        auto sceneData = std::make_unique<View::Scene>();
        sceneData->value().lightDiffuse = vsg::vec4(0.7, 0.7, 0.7, 1);
        sceneData->value().ambient = vsg::vec4(0.3, 0.3, 0.3, 1);

        auto descriptors = View::createLightingDescriptors(mLightData, false);
        descriptors.emplace_back(View::dummyEnvMap());
        descriptors.emplace_back(View::dummyShadowMap());
        descriptors.emplace_back(View::dummyShadowSampler());
        descriptors.emplace_back(sceneData->descriptor());
        auto descriptorSet = View::createViewDescriptorSet(descriptors);

        auto entry = std::make_unique<ViewData>();
        entry->sceneData = std::move(sceneData);
        entry->descriptorSet = descriptorSet;
        entry->state = View::createDefaultState(descriptorSet);
        // Create a specialized terrain view that contains just the cell we need, and is unaffected by view distance culling of the 3D scene's view distance.
        entry->terrainView = mTerrain->createView();

        if (!mCompile->compile(entry->state))
            return {};

        mViewData.emplace_back(std::move(entry));
        return mViewData.back().get();
    }

    void Map::addCell(MWWorld::CellStore* cell)
    {
        if (cell->isExterior())
        {
            auto& segment = mSegments[std::make_pair(cell->getCell()->getGridX(), cell->getCell()->getGridY())];
            segment.cell = cell;
        }
        else
            clearSegments();
    }

    void Map::removeCell(int x, int y)
    {
        auto found = mSegments.find(std::make_pair(x, y));
        if (found != mSegments.end())
        {
            recycle(found->second);
            mSegments.erase(found);
        }
    }

    void Map::removeCell(MWWorld::CellStore* cell)
    {
        saveFogOfWar(cell);

        if (!cell->isExterior())
            clearSegments();
        else
            removeCell(cell->getCell()->getGridX(), cell->getCell()->getGridY());
    }

    vsg::ImageView* Map::getMapTexture(int x, int y)
    {
        auto found = mSegments.find(std::make_pair(x, y));
        if (found == mSegments.end() || !found->second.mapTexture)
            return {};
        return found->second.mapTexture->imageInfo->imageView;
    }

    vsg::ImageView* Map::getFogOfWarTexture(int x, int y)
    {
        auto found = mSegments.find(std::make_pair(x, y));
        if (found == mSegments.end() || !found->second.fogTexture)
            return {};
        return found->second.fogTexture->imageInfo->imageView;
    }

    void Map::requestExteriorMap(const MWWorld::CellStore* cell)
    {
        int x = cell->getCell()->getGridX();
        int y = cell->getCell()->getGridY();
        Segment& segment = mSegments[std::make_pair(x, y)];
        segment.cell = cell;

        /*
         * Ensures that objects crossing cell borders are contained in existing rendered map.
         */
        for (const auto& [coord, _] : mSegments)
        {
            if (std::abs(coord.first - x) > 1 || std::abs(coord.second - y) > 1)
                continue;
            if (segment.renderedGrid.find(coord) == segment.renderedGrid.end())
                segment.needUpdate = true; // openmw-7140-map-update
        }

        if (!segment.needUpdate)
            return;

        if (!segment.fogTexture)
            loadFogOfWar(segment);

        mInterior = false;

        auto bounds = vsg::visit<vsg::ComputeBounds>(mSceneGraph).bounds;
        float zmin = bounds.min.z;
        float zmax = bounds.max.z;

        auto viewData = setupRenderToTexture(cell->getCell()->getGridX(), cell->getCell()->getGridY(),
            x * mMapWorldSize + mMapWorldSize / 2.f, y * mMapWorldSize + mMapWorldSize / 2.f, vsg::dvec3(0, 1, 0), zmin, zmax);
        if (!viewData)
            return;
        viewData->sceneChildren = { mSceneGraph };
        if (auto compileOp = mTerrain->updateViewForCell(*viewData->terrainView, vsg::ivec2(x, y), mReferenceViewDistance))
        {
            compileOp->compile();
            viewData->sceneChildren.push_back(viewData->terrainView->node());
        }

        segment.needUpdate = false;
        segment.renderedGrid.clear();
        for (const auto& [coord, _] : mSegments)
            segment.renderedGrid.insert(coord);
    }

    void Map::loadFogOfWar(Segment& segment)
    {
        if (mInterior)
            return; // vsgopenmw-fixme
        const auto cell = segment.cell;
        if (!cell)
            return;
        if (!segment.fogTexture)
            segment.fogTexture = getOrCreateFogTexture();
        if (segment.fogTexture)
        {
            if (!cell->getFog() || !segment.loadFogOfWar(cell->getFog()->mFogTextures.back()))
            {
                segment.clearFogOfWar();
                mUpdateFog->clearRequests.push_back({ segment.fogTexture->imageInfo->imageView->image });
            }
        }
    }

    void Map::requestInteriorMap(const MWWorld::CellStore* cell)
    {
        auto north = getNorthVector(cell);
        auto bounds = vsg::visit<vsg::ComputeBounds>(mSceneGraph).bounds;

        // If we're in an empty cell, bail out
        // The operations in this function are only valid for finite bounds
        if (!bounds.valid())
            return;

        mInterior = true;

        mBounds = bounds;

        // vsgopenmw-rewrite-me
        // The following code for dividing interior cells is messy, breaks under certain circumstances and doesn't support lazy-loading of individual segments.

        // Get the cell's NorthMarker rotation. This is used to rotate the entire map.
        mAngle = std::atan2(north.x, north.y);

        // Rotate the cell and merge the rotated corners to the bounding box
        auto origCenter3d = vsgUtil::center(bounds);
        vsg::dvec2 origCenter(origCenter3d.x, origCenter3d.y);
        vsg::dvec3 origCorners[8];
        for (int i = 0; i < 8; ++i)
            origCorners[i] = vsgUtil::corner(i, bounds);

        for (int i = 0; i < 8; ++i)
        {
            auto corner = origCorners[i];
            vsg::dvec2 corner2d(corner.x, corner.y);
            corner2d = rotatePoint(corner2d, origCenter, mAngle);
            mBounds.add(vsg::dvec3(corner2d.x, corner2d.y, 0));
        }

        // Do NOT change padding! This will break older savegames.
        // If the padding really needs to be changed, then it must be saved in the ESM::FogState and
        // assume the old (500) value as default for older savegames.
        const float padding = 500.0f;

        // Apply a little padding
        // vsgopenmw-fixme Why?
        mBounds.min -= vsg::dvec3(padding, padding, 0.f);
        mBounds.max += vsg::dvec3(padding, padding, 0.f);

        float zMin = mBounds.min.z;
        float zMax = mBounds.max.z;

        // vsgopenmw-fixme The following comment doesn't match the code that follows
        // If there is fog state in the CellStore (e.g. when it came from a savegame) we need to do some checks
        // to see if this state is still valid.
        // Both the cell bounds and the NorthMarker rotation could be changed by the content files or exchanged models.
        // If they changed by too much then parts of the interior might not be covered by the map anymore.
        // The following code detects this, and discards the CellStore's fog state if it needs to.
        std::vector<std::pair<int, int>> segmentMappings;
        if (ESM::FogState* fog = cell->getFog())
        {
            if (std::abs(mAngle - fog->mNorthMarkerAngle) < vsg::radians(5.f))
            {
                // Expand mBounds so the saved textures fit the same grid
                // vsgopenmw-fixme The following code is incomplete or broken. It consistently breaks the map on save load when epsilon is removed.
                int xOffset = 0;
                int yOffset = 0;
                const float epsilon = 0.01f;
                if (fog->mBounds.mMinX + epsilon < mBounds.min.x)
                {
                    mBounds.min.x = fog->mBounds.mMinX;
                }
                else if (fog->mBounds.mMinX > mBounds.min.x + epsilon)
                {
                    float diff = fog->mBounds.mMinX - mBounds.min.x;
                    xOffset += diff / mMapWorldSize;
                    xOffset++;
                    mBounds.min.x = fog->mBounds.mMinX - xOffset * mMapWorldSize;
                }
                if (fog->mBounds.mMinY + epsilon < mBounds.min.y)
                {
                    mBounds.min.y = fog->mBounds.mMinY;
                }
                else if (fog->mBounds.mMinY > mBounds.min.y + epsilon)
                {
                    float diff = fog->mBounds.mMinY - mBounds.min.y;
                    yOffset += diff / mMapWorldSize;
                    yOffset++;
                    mBounds.min.y = fog->mBounds.mMinY - yOffset * mMapWorldSize;
                }
                if (fog->mBounds.mMaxX > mBounds.max.x)
                    mBounds.max.x = fog->mBounds.mMaxX;
                if (fog->mBounds.mMaxY > mBounds.max.y)
                    mBounds.max.y = fog->mBounds.mMaxY;

                const auto& textures = fog->mFogTextures;
                segmentMappings.reserve(textures.size());
                vsg::dbox savedBounds(vsg::dvec3(fog->mBounds.mMinX, fog->mBounds.mMinY, 0),
                    vsg::dvec3(fog->mBounds.mMaxX, fog->mBounds.mMaxY, 0));
                auto segments = divideIntoSegments(savedBounds, mMapWorldSize);
                for (int x = 0; x < segments.first; ++x)
                    for (int y = 0; y < segments.second; ++y)
                        segmentMappings.emplace_back(std::make_pair(x + xOffset, y + yOffset));

                mAngle = fog->mNorthMarkerAngle;
            }
        }

        vsg::dvec2 min(mBounds.min.x, mBounds.min.y);

        auto center3d = vsgUtil::center(mBounds);
        vsg::dvec2 center(center3d.x, center3d.y);
        vsg::dquat cameraOrient(mAngle, vsg::dvec3(0, 0, -1));

        auto segments = divideIntoSegments(mBounds, mMapWorldSize);
        for (int x = 0; x < segments.first; ++x)
        {
            for (int y = 0; y < segments.second; ++y)
            {
                vsg::dvec2 start = min + vsg::dvec2(mMapWorldSize * x, mMapWorldSize * y);
                vsg::dvec2 newcenter = start + vsg::dvec2(mMapWorldSize / 2.f, mMapWorldSize / 2.f);

                auto a = newcenter - center;
                auto rotatedCenter = cameraOrient * vsg::dvec3(a.x, a.y, 0);

                auto pos = vsg::dvec2(rotatedCenter.x, rotatedCenter.y) + center;

                auto viewData = setupRenderToTexture(x, y, pos.x, pos.y, vsg::dvec3(north.x, north.y, 0), zMin, zMax);
                if (!viewData)
                    continue;
                viewData->sceneChildren = { mSceneGraph };

                auto coords = std::make_pair(x, y);
                Segment& segment = mSegments[coords];
                segment.cell = cell;
                if (!segment.fogTexture)
                    segment.fogTexture = getOrCreateFogTexture();
                if (segment.fogTexture)
                {
                    bool loaded = false;
                    if (ESM::FogState* fog = cell->getFog())
                    {
                        for (size_t index{}; index < segmentMappings.size(); index++)
                        {
                            if (segmentMappings[index] == coords)
                            {
                                if (segment.loadFogOfWar(fog->mFogTextures[index]))
                                    loaded = true;
                                break;
                            }
                        }
                    }
                    if (!loaded)
                    {
                        segment.clearFogOfWar();
                        mUpdateFog->clearRequests.push_back({ segment.fogTexture->imageInfo->imageView->image });
                    }
                }
            }
        }
    }

    void Map::worldToInteriorMapPosition(vsg::dvec2 pos, float& nX, float& nY, int& x, int& y)
    {
        auto center = vsgUtil::center(mBounds);
        pos = rotatePoint(pos, vsg::dvec2(center.x, center.y), mAngle);

        vsg::dvec2 min(mBounds.min.x, mBounds.min.y);

        x = static_cast<int>(std::ceil((pos.x - min.x) / mMapWorldSize) - 1);
        y = static_cast<int>(std::ceil((pos.y - min.y) / mMapWorldSize) - 1);

        nX = (pos.x - min.x - mMapWorldSize * x) / mMapWorldSize;
        nY = (pos.y - min.y - mMapWorldSize * y) / mMapWorldSize;
    }

    vsg::dvec2 Map::interiorMapToWorldPosition(float nX, float nY, int x, int y)
    {
        vsg::dvec2 min(mBounds.min.x, mBounds.min.y);
        vsg::dvec2 pos(mMapWorldSize * (nX + x) + min.x, mMapWorldSize * (nY + y) + min.y);

        auto center = vsgUtil::center(mBounds);
        pos = rotatePoint(pos, vsg::dvec2(center.x, center.y), -mAngle);
        return pos;
    }

    bool Map::isPositionExplored(float nX, float nY, int x, int y) const
    {
        const auto found = mSegments.find(std::make_pair(x, y));
        if (found == mSegments.end())
            return false;
        const auto& segment = found->second;
        if (!segment.fogTexture || !segment.fogTexture->data)
            return false;

        nX = std::clamp(nX, 0.f, 1.f);
        nY = std::clamp(nY, 0.f, 1.f);

        vsg::ivec2 coord(static_cast<int>((FogConstants::resolution - 1) * nX),
            static_cast<int>((FogConstants::resolution - 1) * nY));
        uint8_t alpha = (*segment.fogTexture->data)(coord.x, coord.y);
        return alpha < 200;
    }

    bool Map::isCellExplored(int x, int y) const
    {
        if (!mInterior)
        {
            const auto found = mSegments.find(std::make_pair(x, y));
            if (found != mSegments.end())
                return found->second.fogTexture != nullptr;
            else
                return false;
        }
        else
            return true; // vsgopenmw-fixme
    }

    vsg::Node* Map::getNode()
    {
        return mBlit;
    }

    vsg::Node* Map::getFogNode()
    {
        return mUpdateFog;
    }

    void Map::updatePlayer(const vsg::vec3& position, const vsg::quat& orientation, float& u, float& v, int& x, int& y,
        vsg::vec3& direction)
    {
        // retrieve the x,y grid coordinates the player is in
        vsg::dvec2 pos(position.x, position.y);

        if (mInterior)
        {
            worldToInteriorMapPosition(pos, u, v, x, y);

            // vsg::quat cameraOrient (mAngle, vsg::vec3(0,0,-1));
            // direction = orientation * cameraOrient.inverse() * osg::Vec3f(0,1,0);
            vsg::quat cameraOrient(-mAngle, vsg::vec3(0, 0, -1));
            direction = (orientation * cameraOrient) * vsg::vec3(0, 1, 0);
        }
        else
        {
            direction = orientation * vsg::vec3(0, 1, 0);

            x = static_cast<int>(std::ceil(pos.x / mMapWorldSize) - 1);
            y = static_cast<int>(std::ceil(pos.y / mMapWorldSize) - 1);

            // convert from world coordinates to texture UV coordinates
            u = std::abs((pos.x - (mMapWorldSize * x)) / mMapWorldSize);
            v = std::abs((pos.y - (mMapWorldSize * y)) / mMapWorldSize);
        }

        const float exploreRadius = FogConstants::exploreRadius;
        const float sqrExploreRadius = square(exploreRadius);
        const float exploreRadiusUV = exploreRadius / FogConstants::resolution; // explore radius from 0 to 1 (UV space)

        // change the affected fog of war textures (in a 3x3 grid around the player)
        for (int mx = -mCellDistance; mx <= mCellDistance; ++mx)
        {
            for (int my = -mCellDistance; my <= mCellDistance; ++my)
            {
                // is this texture affected at all?
                bool affected = false;
                if (mx == 0 && my == 0) // the player is always in the center of the 3x3 grid
                    affected = true;
                else
                {
                    bool affectsX = (mx > 0) ? (u + exploreRadiusUV > 1) : (u - exploreRadiusUV < 0);
                    bool affectsY = (my > 0) ? (v + exploreRadiusUV > 1) : (v - exploreRadiusUV < 0);
                    affected = (affectsX && (my == 0)) || (affectsY && mx == 0) || (affectsX && affectsY);
                }
                if (!affected)
                    continue;

                int texX = x + mx;
                int texY = y + my;

                Segment& segment = mSegments[std::make_pair(texX, texY)];
                if (!segment.fogTexture)
                    loadFogOfWar(segment);
                if (!segment.fogTexture)
                    continue;

                auto& tex = *segment.fogTexture;
                vsg::vec2 center(-mx * FogConstants::resolution + u * FogConstants::resolution,
                    -my * FogConstants::resolution + v * FogConstants::resolution);

                vsg::vec4 rect(
                    std::max(0.f, std::floor(center.x - exploreRadius)),
                    std::max(0.f, std::floor(center.y - exploreRadius)),
                    std::min(static_cast<float>(FogConstants::resolution), std::ceil(center.x + exploreRadius)),
                    std::min(static_cast<float>(FogConstants::resolution), std::ceil(center.y + exploreRadius)));

                for (int texU = rect.x; texU < rect.z; ++texU)
                {
                    for (int texV = rect.y; texV < rect.w; ++texV)
                    {
                        //float sqrDist = square(texU - center.x) + square(texV - center.y);
                        // openmw-7373-fog-of-war-borders
                        float sqrDist = square(texU + 0.5 - center.x) + square(texV + 0.5 - center.y);
                        auto& val = (*tex.data)(texU, texV);
                        val = std::min(
                            val, static_cast<uint8_t>(std::clamp(sqrDist / sqrExploreRadius, 0.f, 1.f) * 255));
                    }
                }

                /*
                 * Reduces transfer operations by recomputing updated region on the GPU.
                 */
                // if (!useComputeShader) tex.data->dirty(); else
                mUpdateFog->requests.push_back({ tex.bds, tex.imageInfo->imageView->image, rect, center }); // vsgopenmw-optimization-mwrender-map-compute-fog

                segment.hasFogState = true;
            }
        }
    }

    void Map::Segment::clearFogOfWar()
    {
        std::memset(fogTexture->data->dataPointer(), 0xff, fogTexture->data->dataSize());
    }

    bool Map::Segment::loadFogOfWar(const ESM::FogTexture& esm)
    {
        if (esm.mImageData.empty())
            return false;

        if (auto data = vsgUtil::readImageFromMemory(esm.mImageData))
        {
            if (auto swizzled_data = data->cast<vsg::ubvec4Array2D>())
            {
                fogTexture->data->dirty();
                for (uint32_t x = 0; x < FogConstants::resolution; ++x)
                    for (uint32_t y = 0; y < FogConstants::resolution; ++y)
                        (*fogTexture->data)(x, y) = (*swizzled_data)(x, FogConstants::resolution - y - 1).a;
                hasFogState = true;
                return true;
            }
        }
        return false;
    }

    void Map::Segment::saveFogOfWar(ESM::FogTexture& fog) const
    {
        auto data = fogTexture->data;
        if (!data)
            return;

        // extra flips are unfortunate, but required for compatibility with older versions
        auto swizzled_data = vsg::ubvec4Array2D::create(FogConstants::resolution, FogConstants::resolution,
            vsg::ubvec4(0, 0, 0, 0), vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM });
        for (uint32_t x = 0; x < FogConstants::resolution; ++x)
            for (uint32_t y = 0; y < FogConstants::resolution; ++y)
                (*swizzled_data)(x, FogConstants::resolution - y - 1).a = (*data)(x, y);

        fog.mImageData = vsgUtil::writeImageToMemory(swizzled_data);
    }

}

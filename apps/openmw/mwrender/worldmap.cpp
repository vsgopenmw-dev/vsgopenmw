#include "worldmap.hpp"

#include <climits>

#include <vsg/commands/Commands.h>
#include <vsg/io/Options.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/PushConstants.h>
#include <vsg/threading/OperationThreads.h>
#include <vsg/vk/Context.h>

#include <components/esm3/globalmap.hpp>
#include <components/pipeline/sets.hpp>
#include <components/pipeline/worldoverlay.hpp>
#include <components/render/computeimage.hpp>
#include <components/render/screenshot.hpp>
#include <components/render/attachmentformat.hpp>
#include <components/vsgutil/image.hpp>
#include <components/vsgutil/imageio.hpp>
#include <components/vsgutil/share.hpp>
#include <components/vsgutil/deletionqueue.hpp>
#include <components/vsgutil/compilecontext.hpp>

#include "../mwworld/esmstore.hpp"

namespace MWRender
{
    struct WorldMap::CreateMap : public vsgUtil::Operation
    {
        CreateMap(int width, int height, const Bounds& bounds, int cellSize,
            const MWWorld::Store<ESM::Land>& landStore)
            : mWidth(width)
            , mHeight(height)
            , mBounds(bounds)
            , mCellSize(cellSize)
            , mLandStore(landStore)
        {
        }

        void operate() override
        {
            // note, RGB image data has to be converted to RGBA when copying to a VkImage,
            // this makes RGB substantially slower than using RGBA data.
            //mBaseData = vsg::ubvec3Array2D::create(mWidth, mHeight, vsg::Data::Properties{ VK_FORMAT_R8G8B8_UNORM });
            mBaseData = vsg::ubvec4Array2D::create(mWidth, mHeight, vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM });
            mAlphaData = vsg::ubyteArray2D::create(mWidth, mHeight, vsg::Data::Properties{ VK_FORMAT_R8_UNORM });

            for (int x = mBounds.minX; x < mBounds.maxX; ++x)
            {
                for (int y = mBounds.minY; y < mBounds.maxY; ++y)
                {
                    const ESM::Land* land = mLandStore.search(x, y);

                    for (int cellY = 0; cellY < mCellSize; ++cellY)
                    {
                        for (int cellX = 0; cellX < mCellSize; ++cellX)
                        {
                            int vertexX = static_cast<int>(float(cellX) / float(mCellSize) * 9);
                            int vertexY = static_cast<int>(float(cellY) / float(mCellSize) * 9);

                            int texelX = (x - mBounds.minX) * mCellSize + cellX;
                            int texelY = (y - mBounds.minY) * mCellSize + cellY;

                            unsigned char r, g, b;

                            float y2 = 0;
                            if (land && (land->mDataTypes & ESM::Land::DATA_WNAM))
                                y2 = land->mWnam[vertexY * 9 + vertexX] / 128.f;
                            else
                                y2 = SCHAR_MIN / 128.f;
                            if (y2 < 0)
                            {
                                r = static_cast<unsigned char>(14 * y2 + 38);
                                g = static_cast<unsigned char>(20 * y2 + 56);
                                b = static_cast<unsigned char>(18 * y2 + 51);
                            }
                            else if (y2 < 0.3f)
                            {
                                if (y2 < 0.1f)
                                    y2 *= 8.f;
                                else
                                {
                                    y2 -= 0.1f;
                                    y2 += 0.8f;
                                }
                                r = static_cast<unsigned char>(66 - 32 * y2);
                                g = static_cast<unsigned char>(48 - 23 * y2);
                                b = static_cast<unsigned char>(33 - 16 * y2);
                            }
                            else
                            {
                                y2 -= 0.3f;
                                y2 *= 1.428f;
                                r = static_cast<unsigned char>(34 - 29 * y2);
                                g = static_cast<unsigned char>(25 - 20 * y2);
                                b = static_cast<unsigned char>(17 - 12 * y2);
                            }

                            (*mBaseData)[texelY * mWidth + texelX] = vsg::ubvec4{ r, g, b, 0xff };
                            (*mAlphaData)[texelY * mWidth + texelX]
                                = (y2 < 0) ? static_cast<unsigned char>(0) : static_cast<unsigned char>(255);
                        }
                    }
                }
            }
        }

        int mWidth, mHeight;
        Bounds mBounds;
        int mCellSize;
        const MWWorld::Store<ESM::Land>& mLandStore;

        vsg::ref_ptr<vsg::ubvec4Array2D> mBaseData;
        vsg::ref_ptr<vsg::ubyteArray2D> mAlphaData;
    };

    struct WorldMap::Save : public Render::Screenshot
    {
        std::vector<char> mEncoded;
        Save(vsg::ref_ptr<vsg::Image> image, vsg::Device* device)
            : Screenshot(device, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
        }
        void operate() override
        {
            Screenshot::operate();
            if (downloadData)
                mEncoded = vsgUtil::writeImageToMemory(downloadData);
        }
    };

    class WorldMap::UpdateOverlay : public Render::ComputeImage
    {
        vsg::ref_ptr<vsg::vec4Array> mArgs = vsg::vec4Array::create(1);
        mutable vsgUtil::DeletionQueue mDeletionQueue = vsgUtil::DeletionQueue(4);

    public:
        vsg::PipelineLayout* layout{};

        UpdateOverlay()
        {
            mClearColorImage->color = { { 0, 0, 0, 0 } };
        }
        void createComputeGraph(vsg::ref_ptr<vsg::BindComputePipeline> pipeline, vsg::ref_ptr<vsg::ImageView> alphaTexture, vsg::ref_ptr<vsg::Image> overlayImage)
        {
            layout = pipeline->pipeline->layout;
            auto pc = vsg::PushConstants::create(VK_SHADER_STAGE_COMPUTE_BIT, 0, mArgs);

            auto storageImageView = vsg::ImageView::create(overlayImage);
            auto storageImage = vsg::DescriptorImage::create(
                vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), storageImageView, VK_IMAGE_LAYOUT_GENERAL), 0, 0,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            auto maskTexture
                = vsg::DescriptorImage::create(vsg::ImageInfo::create(vsgUtil::shareDefault<vsg::Sampler>(),
                    alphaTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                    1);

            auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, layout, Pipeline::TEXTURE_SET,
                vsg::Descriptors{ storageImage, maskTexture });

            auto commands = vsg::Commands::create();
            commands->children = { pipeline, bds, pc, mDispatch };
            children = { commands };
        }
        struct Request
        {
            vsg::ref_ptr<vsg::BindDescriptorSet> bds;
            vsg::ref_ptr<vsg::Image> image;
            vsg::vec4 dstRect;
        };
        mutable std::vector<Request> requests;
        void accept(vsg::RecordTraversal& visitor) const override
        {
            mDeletionQueue.advanceFrame();
            if (requests.empty() && clearRequests.empty())
                return;

            handleClearRequests(visitor);

            for (auto& r : requests)
            {
                mArgs->at(0) = r.dstRect;
                visitor.apply(*r.bds);
                mDeletionQueue.add(r.bds);

                #include <files/shaders/comp/worldoverlay/workgroupsize.glsl>                           // Vulkan guarantees a minimum of 128 maxComputeWorkGroupInvocations.
                static_assert(workGroupSize * workGroupSize <= 128);
                compute(visitor, r.image, r.dstRect.z - r.dstRect.x, r.dstRect.w - r.dstRect.y, workGroupSize);
            }
            requests.clear();
        }
        void requestClear(vsg::ref_ptr<vsg::Image> image)
        {
            requests.clear();
            clearRequests.push_back({ image });
        }
    };

    WorldMap::WorldMap(vsg::ref_ptr<vsgUtil::CompileContext> compile, const vsg::Options* shaderOptions, vsg::ref_ptr<vsg::OperationThreads> threads)
        : mOperationThreads(threads)
        , mCompile(compile)
        , mShaderOptions(shaderOptions)
    {
        mUpdate = new UpdateOverlay();
    }

    void WorldMap::load(const MWWorld::ESMStore& esmStore, int cellSize)
    {
        mCellSize = cellSize;

        // get the size of the world
        // auto& store = esmStore.get<ESM::Cell>
        // openmw-7192-world-map-size
        auto& store = esmStore.get<ESM::Land>();
        for (auto it = store.begin(); it != store.end(); ++it)
            mBounds.addCellArea(it->mX, it->mY);

        mWidth = mCellSize * (mBounds.maxX - mBounds.minX);
        mHeight = mCellSize * (mBounds.maxY - mBounds.minY);

        mCreateMap = new CreateMap(mWidth, mHeight, mBounds, mCellSize, esmStore.get<ESM::Land>());
        mOperationThreads->add(mCreateMap);

        mOverlayTexture = vsgUtil::createImageAndView(Render::compatibleColorFormat, VkExtent2D{ mWidth, mHeight },
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        mAlphaTexture = vsgUtil::createImageAndView(VK_FORMAT_R8_UNORM, VkExtent2D{ mWidth, mHeight }, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    }

    WorldMap::~WorldMap()
    {
        if (mSave)
            mSave->wait();
        if (mCreateMap)
            mCreateMap->wait();
    }

    vsg::Node* WorldMap::node()
    {
        return mUpdate;
    }

    void WorldMap::worldPosToImageSpace(float x, float y, float& imageX, float& imageY) const
    {
        imageX = (float(x / float(Constants::CellSizeInUnits) - mBounds.minX) / (mBounds.maxX - mBounds.minX)) * getWidth();

        imageY = (1.f - float(y / float(Constants::CellSizeInUnits) - mBounds.minY) / (mBounds.maxY - mBounds.minY)) * getHeight();
    }

    void WorldMap::exploreCell(int cellX, int cellY, vsg::ImageView* localMapTexture)
    {
        if (!localMapTexture)
            return;

        ensureLoaded();

        if (cellX >= mBounds.maxX || cellX < mBounds.minX || cellY >= mBounds.maxY || cellY < mBounds.minY)
            return;

        mExploredBounds.addCellArea(cellX, cellY);
        int originX = (cellX - mBounds.minX) * mCellSize;
        int originY = (cellY - mBounds.minY) * mCellSize;

        auto descriptor = vsg::DescriptorImage::create(
            vsg::ImageInfo::create(vsgUtil::shareDefault<vsg::Sampler>(), vsg::ref_ptr{ localMapTexture },
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
            0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        auto bds = vsg::BindDescriptorSet::create(
            VK_PIPELINE_BIND_POINT_COMPUTE, mUpdate->layout, Pipeline::COMPUTE_SET, vsg::Descriptors{ descriptor });
        if (mCompile->compile(bds))
            mUpdate->requests.push_back({ bds, mOverlayTexture->image, vsg::vec4(originX, originY, originX + mCellSize, originY + mCellSize) });
    }

    void WorldMap::clear()
    {
        mExploredBounds = {};
        if (!mCreateMap)
            mUpdate->requestClear(mOverlayTexture->image);
    }

    void WorldMap::write(ESM::GlobalMap& map)
    {
        if (!mExploredBounds.valid() || mCreateMap)
            return;

        mExploredBounds.write(map);

        if (mSave)
        {
            mSave->wait();
            map.mImageData = std::move(mSave->mEncoded);
            mSave = {};
        }
    }

    vsg::ivec4 WorldMap::cropImageRect(const Bounds& srcBounds, const Bounds& dstBounds, int cellWidth, int cellHeight, int imageWidth, int imageHeight) const
    {
        int leftDiff = (dstBounds.minX - srcBounds.minX);
        int topDiff = (dstBounds.minY - srcBounds.minY);
        int bottomDiff = (srcBounds.maxY - dstBounds.maxY);
        int rightDiff = (srcBounds.maxX - dstBounds.maxX);
        return {
            std::max(0, leftDiff * cellWidth),
            std::max(0, topDiff * cellHeight),
            std::min(imageWidth, imageWidth - rightDiff * cellWidth),
            std::min(imageHeight, imageHeight - bottomDiff * cellHeight)
        };
    }

    void WorldMap::read(ESM::GlobalMap& map)
    {
        auto savedBounds = Bounds::read(map);
        if (!savedBounds.valid() || map.mImageData.empty())
            return;

        mExploredBounds = Bounds::intersect(mBounds, savedBounds);
        if (!mExploredBounds.valid())
            return;

        auto data = vsgUtil::readImageFromMemory(map.mImageData);
        if (!data)
            return;
        auto image = vsg::ref_ptr{ data->cast<vsg::ubvec4Array2D>() };
        if (!image)
            return;

        ensureLoaded();

        uint32_t imageWidth = image->width();
        uint32_t imageHeight = image->height();

        // Size of one cell in image space
        int cellWidthSrc = imageWidth / (savedBounds.maxX - savedBounds.minX);
        int cellHeightSrc = imageHeight / (savedBounds.maxY - savedBounds.minY);

       // If cell bounds of the currently loaded content and the loaded savegame do not match,
        // we need to resize source/dest boxes to accommodate
        // This means nonexisting cells will be dropped silently
        vsg::ivec4 srcRect = cropImageRect(savedBounds, mBounds, cellWidthSrc, cellHeightSrc, imageWidth, imageHeight);
        vsg::ivec4 dstRect = cropImageRect(mBounds, savedBounds, mCellSize, mCellSize, mWidth, mHeight);

        // In case of a changed map resolution, we'll want filtering.
        // Draw the image.
        auto drawData = image;
        if (srcRect.x != 0 || srcRect.y != 0 || srcRect.z != static_cast<int>(imageWidth) || srcRect.w != static_cast<int>(imageHeight))
        {
            uint32_t w = srcRect.z - srcRect.x;
            uint32_t h = srcRect.w - srcRect.y;
            drawData = vsg::ubvec4Array2D::create(w, h, vsg::Data::Properties{ VK_FORMAT_R8G8B8A8_UNORM });
            for (uint32_t x = 0; x < w; ++x)
                for (uint32_t y = 0; y < h; ++y)
                    (*drawData)(x, y) = (*image)(x + srcRect.x, y + srcRect.y);
        }

        auto descriptor = vsg::DescriptorImage::create(vsgUtil::shareDefault<vsg::Sampler>(), drawData);
        auto bds = vsg::BindDescriptorSet::create(
            VK_PIPELINE_BIND_POINT_COMPUTE, mUpdate->layout, Pipeline::COMPUTE_SET, vsg::Descriptors{ descriptor });
        if (mCompile->compile(bds))
            mUpdate->requests.push_back({ bds, mOverlayTexture->image, vsg::vec4(dstRect) });
    }

    vsg::ImageView* WorldMap::getOverlayTexture()
    {
        ensureLoaded();
        return mOverlayTexture;
    }

    void WorldMap::ensureLoaded()
    {
        if (mCreateMap)
        {
            mCreateMap->ensure();

            mBaseData = mCreateMap->mBaseData;
            mAlphaTexture->image->data = mCreateMap->mAlphaData;

            mUpdate->createComputeGraph(Pipeline::worldOverlay(mShaderOptions), mAlphaTexture, mOverlayTexture->image);
            if (!mCompile->compile(mUpdate))
                mUpdate->children.clear();
            else
                mUpdate->requestClear(mOverlayTexture->image);

            mCreateMap = {};
        }
    }

    void WorldMap::asyncSave()
    {
        if (mCreateMap)
            return;

        vsg::ivec4 exploredRect = cropImageRect(mBounds, mExploredBounds, mCellSize, mCellSize, mWidth, mHeight);

        mSave = new Save(mOverlayTexture->image, mCompile->device);
        // mSave->srcRect = { 0, 0, mWidth, mHeight };
        // openmw-7192-world-map-size
        // vsgopenmw-optimization-mwrender-worldmap-save-explored-region
        mSave->srcRect = {
            { static_cast<int32_t>(exploredRect.x), static_cast<int32_t>(exploredRect.y) },
            { static_cast<uint32_t>(exploredRect.z - exploredRect.x), static_cast<uint32_t>(exploredRect.w - exploredRect.y) }};

        mOperationThreads->add(mSave);
    }

    vsg::ref_ptr<vsg::Data> WorldMap::getBaseTexture()
    {
        ensureLoaded();
        return mBaseData;
    }

    bool WorldMap::Bounds::valid() const
    {
        return minX < maxX && minY < maxY;
    }

    WorldMap::Bounds WorldMap::Bounds::intersect(const Bounds& lhs, const Bounds& rhs)
    {
        Bounds intersection{
            std::max(lhs.minX, rhs.minX),
            std::min(lhs.maxX, rhs.maxX),
            std::max(lhs.minY, rhs.minY),
            std::min(lhs.maxY, rhs.maxY)
        };
        if (intersection.valid())
            return intersection;
        return {};
    }

    void WorldMap::Bounds::addCellArea(int cellX, int cellY)
    {
        minX = std::min(minX, cellX);
        maxX = std::max(maxX, cellX+1);
        minY = std::min(minY, cellY);
        maxY = std::max(maxY, cellY+1);
    }

    WorldMap::Bounds WorldMap::Bounds::read(const ESM::GlobalMap& esm)
    {
        if (esm.mBounds.mMinX > esm.mBounds.mMaxX || esm.mBounds.mMinY > esm.mBounds.mMaxY)
            return {};
        return { esm.mBounds.mMinX, esm.mBounds.mMaxX+1, esm.mBounds.mMinY, esm.mBounds.mMaxY+1 };
    }

    void WorldMap::Bounds::write(ESM::GlobalMap& esm) const
    {
        esm.mBounds.mMinX = minX;
        esm.mBounds.mMaxX = maxX-1;
        esm.mBounds.mMinY = minY;
        esm.mBounds.mMaxY = maxY-1;
    }
}

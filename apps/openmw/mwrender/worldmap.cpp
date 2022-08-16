#include "worldmap.hpp"

#include <iostream>
#include <climits>

#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/vk/Context.h>
#include <vsg/io/Options.h>
#include <vsg/threading/OperationThreads.h>
#include <vsg/commands/PushConstants.h>
#include <vsg/commands/Commands.h>

#include <components/vsgutil/share.hpp>
#include <components/pipeline/sets.hpp>
#include <components/pipeline/worldoverlay.hpp>
#include <components/esm3/globalmap.hpp>
#include <components/render/screenshot.hpp>
#include <components/render/computeimage.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

#include "imageio.hpp"

namespace MWRender
{
    struct WorldMap::CreateMap : public vsg::Operation
    {
        CreateMap(int width, int height, int minX, int minY, int maxX, int maxY, int cellSize, const MWWorld::Store<ESM::Land>& landStore)
            : mWidth(width), mHeight(height), mMinX(minX), mMinY(minY), mMaxX(maxX), mMaxY(maxY), mCellSize(cellSize), mLandStore(landStore)
        {
        }

        void run() override
        {
            mBaseData = vsg::ubvec3Array2D::create(mWidth, mHeight, vsg::Data::Layout{.format=VK_FORMAT_R8G8B8_UNORM});
            mAlphaData = vsg::ubyteArray2D::create(mWidth, mHeight, vsg::Data::Layout{.format=VK_FORMAT_R8_UNORM});

            for (int x = mMinX; x <= mMaxX; ++x)
            {
                for (int y = mMinY; y <= mMaxY; ++y)
                {
                    const ESM::Land* land = mLandStore.search (x,y);

                    for (int cellY=0; cellY<mCellSize; ++cellY)
                    {
                        for (int cellX=0; cellX<mCellSize; ++cellX)
                        {
                            int vertexX = static_cast<int>(float(cellX) / float(mCellSize) * 9);
                            int vertexY = static_cast<int>(float(cellY) / float(mCellSize) * 9);

                            int texelX = (x-mMinX) * mCellSize + cellX;
                            int texelY = (y-mMinY) * mCellSize + cellY;

                            unsigned char r,g,b;

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

                            (*mBaseData)[texelY * mWidth + texelX] = vsg::ubvec3{r,g,b};
                            (*mAlphaData)[texelY * mWidth + texelX] = (y2 < 0) ? static_cast<unsigned char>(0) : static_cast<unsigned char>(255);
                        }
                    }
                }
            }
        }

        int mWidth, mHeight;
        int mMinX, mMinY, mMaxX, mMaxY;
        int mCellSize;
        const MWWorld::Store<ESM::Land>& mLandStore;

        vsg::ref_ptr<vsg::ubvec3Array2D> mBaseData;
        vsg::ref_ptr<vsg::ubyteArray2D> mAlphaData;
    };

    struct WorldMap::Save : public Render::Screenshot
    {
        std::vector<char> mEncoded;
        Save(vsg::ref_ptr<vsg::Image> image, vsg::ref_ptr<vsg::Device> device) : Screenshot(device, image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {}
        void run() override
        {
            Screenshot::run();
            if (downloadData)
                mEncoded = writeImageToMemory(downloadData);
        }
    };

    class WorldMap::UpdateOverlay : public Render::ComputeImage
    {
        vsg::ref_ptr<vsg::vec4Array> mArgs = vsg::vec4Array::create(1);
        vsg::ref_ptr<vsg::Image> mOverlayImage;
    public:
        vsg::PipelineLayout *const layout;
        UpdateOverlay(vsg::ref_ptr<vsg::BindComputePipeline> pipeline, vsg::ref_ptr<vsg::Image> overlayImage, vsg::ref_ptr<vsg::ImageView> alphaTexture)
            : mOverlayImage(overlayImage)
            , layout(pipeline->pipeline->layout)
        {
            mClearColor = {{0,0,0,0}};
            auto pc = vsg::PushConstants::create(VK_SHADER_STAGE_COMPUTE_BIT, 0, mArgs);

            auto storageImageView = vsg::ImageView::create(overlayImage);
            auto storageImage = vsg::DescriptorImage::create(vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), storageImageView, VK_IMAGE_LAYOUT_GENERAL), 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            auto maskTexture = vsg::DescriptorImage::create(vsg::ImageInfo::create(vsgUtil::shareDefault<vsg::Sampler>(), alphaTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), 1);

            auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, layout, Pipeline::TEXTURE_SET, vsg::Descriptors{storageImage, maskTexture});

            auto commands = vsg::Commands::create();
            commands->children = {pipeline, bds, pc, mDispatch};
            children = {commands};
        }
        struct Request
        {
            vsg::ref_ptr<vsg::BindDescriptorSet> bds;
            vsg::vec4 dstRect;
        };
        mutable std::vector<Request> requests;
        mutable std::vector<Request> pendingDelete;
        void accept(vsg::RecordTraversal &visitor) const override
        {
            if (requests.empty() && clearRequests.empty())
                return;

            disableDefaultPushConstants(visitor);
            handleClearRequests(visitor);

            for (auto &r : requests)
            {
                mArgs->at(0) = r.dstRect;
                visitor.apply(*r.bds);

                compute(visitor, mOverlayImage, r.dstRect.z - r.dstRect.x, r.dstRect.w - r.dstRect.y);
            }
            pendingDelete.swap(requests); //vsgopenmw-deletion-queue
            requests.clear();
        }
        void requestClear()
        {
            clearRequests.push_back({mOverlayImage});
        }
    };

    WorldMap::WorldMap(vsg::Context &ctx, const vsg::Options *shaderOptions, int cellSize)
        : mCellSize(cellSize)
        , mContext(&ctx)
        , mShaderOptions(shaderOptions)
    {
        const MWWorld::ESMStore &esmStore = MWBase::Environment::get().getWorld()->getStore();

        // get the size of the world
        MWWorld::Store<ESM::Cell>::iterator it = esmStore.get<ESM::Cell>().extBegin();
        for (; it != esmStore.get<ESM::Cell>().extEnd(); ++it)
        {
            if (it->getGridX() < mMinX)
                mMinX = it->getGridX();
            if (it->getGridX() > mMaxX)
                mMaxX = it->getGridX();
            if (it->getGridY() < mMinY)
                mMinY = it->getGridY();
            if (it->getGridY() > mMaxY)
                mMaxY = it->getGridY();
        }

        mWidth = mCellSize*(mMaxX-mMinX+1);
        mHeight = mCellSize*(mMaxY-mMinY+1);

        mCreateMap = new CreateMap(mWidth, mHeight, mMinX, mMinY, mMaxX, mMaxY, mCellSize, esmStore.get<ESM::Land>());
        mOperationThreads = vsg::OperationThreads::create(1);
        mOperationThreads->add(mCreateMap);

        auto overlayImage = vsg::Image::create();
        overlayImage->extent = {mWidth, mHeight, 1};
        overlayImage->format = VK_FORMAT_R8G8B8A8_UNORM;
        overlayImage->arrayLayers = 1;
        overlayImage->mipLevels = 1;
        overlayImage->usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        mOverlayTexture = vsg::ImageView::create(overlayImage);
        auto alphaImage = vsg::Image::create();
        alphaImage->extent = {mWidth, mHeight, 1};
        alphaImage->arrayLayers = 1;
        alphaImage->mipLevels = 1;
        alphaImage->format = VK_FORMAT_R8_UNORM;
        alphaImage->usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        mAlphaTexture = vsg::ImageView::create(alphaImage);

        auto pipeline = Pipeline::worldOverlay(mShaderOptions);
        mUpdate = new UpdateOverlay(pipeline, overlayImage, mAlphaTexture);
    }

    WorldMap::~WorldMap()
    {
        finishThreads();
    }

    void WorldMap::finishThreads()
    {
        if (mOperationThreads)
        {
            mOperationThreads->run();
            mOperationThreads = {};
        }
    }

    vsg::Node *WorldMap::node()
    {
        return mUpdate;
    }

    void WorldMap::worldPosToImageSpace(float x, float z, float& imageX, float& imageY)
    {
        imageX = (float(x / float(Constants::CellSizeInUnits) - mMinX) / (mMaxX - mMinX + 1)) * getWidth();

        imageY = (1.f-float(z / float(Constants::CellSizeInUnits) - mMinY) / (mMaxY - mMinY + 1)) * getHeight();
    }

    void WorldMap::exploreCell(int cellX, int cellY, vsg::ImageView *localMapTexture)
    {
        ensureLoaded();

        if (!localMapTexture)
            return;

        int originX = (cellX - mMinX) * mCellSize;
        int originY = (cellY - mMinY) * mCellSize;

        if (cellX > mMaxX || cellX < mMinX || cellY > mMaxY || cellY < mMinY)
            return;

        auto descriptor = vsg::DescriptorImage::create(vsg::ImageInfo::create(vsgUtil::shareDefault<vsg::Sampler>(), vsg::ref_ptr{localMapTexture}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, mUpdate->layout, Pipeline::COMPUTE_SET, vsg::Descriptors{descriptor});
        bds->compile(*mContext);

        mUpdate->requests.push_back({bds, vsg::vec4(originX, originY, originX+mCellSize, originY+mCellSize)});
    }

    void WorldMap::clear()
    {
        mUpdate->requestClear();
    }

    void WorldMap::write(ESM::GlobalMap& map)
    {
        ensureLoaded();

        map.mBounds.mMinX = mMinX;
        map.mBounds.mMaxX = mMaxX;
        map.mBounds.mMinY = mMinY;
        map.mBounds.mMaxY = mMaxY;

        if (mSave)
        {
            finishThreads();
            map.mImageData = std::move(mSave->mEncoded);
            mSave = {};
        }
    }

    void WorldMap::read(ESM::GlobalMap& map)
    {
        ensureLoaded();

        const auto &bounds = map.mBounds;

        if (bounds.mMaxX-bounds.mMinX < 0)
            return;
        if (bounds.mMaxY-bounds.mMinY < 0)
            return;

        if (bounds.mMinX > bounds.mMaxX
                || bounds.mMinY > bounds.mMaxY)
            throw std::runtime_error("invalid map bounds");

        if (map.mImageData.empty())
            return;

        auto data = readImageFromMemory(map.mImageData);
        if (!data)
            return;
        auto image = vsg::ref_ptr{data->cast<vsg::ubvec4Array2D>()};
        if (!image)
            return;

        uint32_t imageWidth = image->width();
        uint32_t imageHeight = image->height();

        int xLength = (bounds.mMaxX-bounds.mMinX+1);
        int yLength = (bounds.mMaxY-bounds.mMinY+1);

        // Size of one cell in image space
        int cellImageSizeSrc = imageWidth / xLength;
        if (int(imageHeight / yLength) != cellImageSizeSrc)
            throw std::runtime_error("cell size must be quadratic");

        // If cell bounds of the currently loaded content and the loaded savegame do not match,
        // we need to resize source/dest boxes to accommodate
        // This means nonexisting cells will be dropped silently
        int cellImageSizeDst = mCellSize;

        // Completely off-screen? -> no need to blit anything
        if (bounds.mMaxX < mMinX
                || bounds.mMaxY < mMinY
                || bounds.mMinX > mMaxX
                || bounds.mMinY > mMaxY)
            return;

        int leftDiff = (mMinX - bounds.mMinX);
        int topDiff = (bounds.mMaxY - mMaxY);
        int rightDiff = (bounds.mMaxX - mMaxX);
        int bottomDiff =  (mMinY - bounds.mMinY);

        vsg::vec4 srcRect ( std::max(0, leftDiff * cellImageSizeSrc),
                                  std::max(0, topDiff * cellImageSizeSrc),
                                  std::min(imageWidth, imageWidth - rightDiff * cellImageSizeSrc),
                                  std::min(imageHeight, imageHeight - bottomDiff * cellImageSizeSrc));

        vsg::vec4 dstRect ( std::max(0, -leftDiff * cellImageSizeDst),
                                   std::max(0, -topDiff * cellImageSizeDst),
                                   std::min(mWidth, mWidth + rightDiff * cellImageSizeDst),
                                   std::min(mHeight, mHeight + bottomDiff * cellImageSizeDst));

        if (srcRect == dstRect && imageWidth == mWidth && imageHeight == mHeight)
        {
            mContext->copy(image, vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), mOverlayTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        }
        else
        {
            // Dimensions don't match. This could mean a changed map region, or a changed map resolution.
            // In the latter case, we'll want filtering.
            // Draw the image.
            auto w = srcRect.z - srcRect.x;
            auto h = srcRect.w - srcRect.y;
            auto srcData = vsg::ubvec4Array2D::create(w, h, vsg::Data::Layout{.format=VK_FORMAT_R8G8B8A8_UNORM});
            for (uint32_t x=0; x<w; ++x)
                for (uint32_t y=0; y<h; ++y)
                    (*srcData)(x,y) = (*image)(x,y);

            auto descriptor = vsg::DescriptorImage::create(vsgUtil::shareDefault<vsg::Sampler>(), srcData);
            auto bds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, mUpdate->layout, Pipeline::COMPUTE_SET, vsg::Descriptors{descriptor});
            bds->compile(*mContext);
            mUpdate->requests.push_back({bds, dstRect});
        }
    }

    vsg::ImageView *WorldMap::getOverlayTexture()
    {
        ensureLoaded();
        return mOverlayTexture;
    }

    void WorldMap::ensureLoaded()
    {
        if (mCreateMap)
        {
            finishThreads();

            mBaseData = mCreateMap->mBaseData;

            mContext->copy(mCreateMap->mAlphaData, vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>(), mAlphaTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
            clear();

            mCreateMap = {};
        }
    }

    void WorldMap::asyncSave()
    {
        if (mCreateMap)
            return;

        mSave = new Save(mOverlayTexture->image, mContext->device);
        if (!mOperationThreads) mOperationThreads = vsg::OperationThreads::create(1);
        mOperationThreads->add(mSave);
    }

    vsg::ref_ptr<vsg::Data> WorldMap::getBaseTexture()
    {
        ensureLoaded();
        return mBaseData;
    }
}

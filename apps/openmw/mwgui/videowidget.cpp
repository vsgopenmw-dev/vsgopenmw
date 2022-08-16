#include "videowidget.hpp"

#include <extern/osg-ffmpeg-videoplayer/videoplayer.hpp>

#include <MyGUI_RenderManager.h>

#include <components/vfs/manager.hpp>

#include "../mwsound/movieaudiofactory.hpp"

namespace MWGui
{
    VideoWidget::VideoWidget()
    {
    }

    VideoWidget::~VideoWidget()
    {
        if (mTexture)
            MyGUI::RenderManager::getInstance().destroyTexture(mTexture);
    }

    void VideoWidget::playVideo(const std::string &video, const VFS::Manager &vfs)
    {
        mPlayer = std::make_unique<Video::VideoPlayer>();
        mPlayer->setAudioFactory(new MWSound::MovieAudioFactory());
        try
        {
            mPlayer->playVideo(vfs.get("Video/" + video), video);
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to open video: " << e.what() << std::endl;
            return;
        }

        if (!mPlayer->getVideoData())
            return;

        if (mTexture && (mTexture->getWidth() != mPlayer->getVideoWidth() || mTexture->getHeight() != mPlayer->getVideoHeight()))
            MyGUI::RenderManager::getInstance().destroyTexture(mTexture);

        mTexture = MyGUI::RenderManager::getInstance().createTexture("video");
        mTexture->createManual(mPlayer->getVideoWidth(), mPlayer->getVideoHeight(), MyGUI::TextureUsage::Dynamic, MyGUI::PixelFormat::R8G8B8A8);

        setBackgroundImage(mTexture, stretch);
    }

    bool VideoWidget::update()
    {
        bool ret = mPlayer && mPlayer->update();
        if (ret)
        {
            auto data = mPlayer->getVideoData();
            auto dataSize = mTexture->getNumElemBytes()*mTexture->getWidth()*mTexture->getHeight();
            auto dst = reinterpret_cast<unsigned char*>(mTexture->lock(MyGUI::TextureUsage::Write));
            std::memcpy(dst, data, dataSize);
            mTexture->unlock();
        }
        return ret;
    }

    void VideoWidget::stop()
    {
        if (mPlayer)
            mPlayer->close();
    }

    bool VideoWidget::hasAudioStream()
    {
        return mPlayer && mPlayer->hasAudioStream();
    }
}

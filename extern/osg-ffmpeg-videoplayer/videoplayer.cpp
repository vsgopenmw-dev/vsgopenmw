#include "videoplayer.hpp"

#include <iostream>

#include "audiofactory.hpp"
#include "videostate.hpp"

namespace Video
{

VideoPlayer::VideoPlayer()
    : mState(nullptr)
{

}

VideoPlayer::~VideoPlayer()
{
    if(mState)
        close();
}

void VideoPlayer::setAudioFactory(MovieAudioFactory *factory)
{
    mAudioFactory.reset(factory);
}

void VideoPlayer::playVideo(std::unique_ptr<std::istream>&& inputstream, const std::string& name)
{
    if(mState)
        close();

    try {
        mState = new VideoState;
        mState->setAudioFactory(mAudioFactory.get());
        mState->init(std::move(inputstream), name);

        // wait until we have the first picture
        while (mState->video_st && !mState->mDisplayData)
        {
            if (!mState->update())
                break;
        }
    }
    catch(std::exception& e) {
        std::cerr<< "Failed to play video: "<<e.what() <<std::endl;
        close();
    }
}

bool VideoPlayer::update ()
{
    if(mState)
        return mState->update();
    return false;
}

void *VideoPlayer::getVideoData()
{
    if (mState)
        return mState->mDisplayData;
    return nullptr;
}

int VideoPlayer::getVideoWidth()
{
    if (mState)
        return mState->video_ctx->width;
    return 0;
}

int VideoPlayer::getVideoHeight()
{
    if (mState)
        return mState->video_ctx->height;
    return 0;
}

void VideoPlayer::close()
{
    if(mState)
    {
        mState->deinit();

        delete mState;
        mState = nullptr;
    }
}

bool VideoPlayer::hasAudioStream()
{
    return mState && mState->audio_st != nullptr;
}

void VideoPlayer::play()
{
    if (mState)
        mState->setPaused(false);
}

void VideoPlayer::pause()
{
    if (mState)
        mState->setPaused(true);
}

bool VideoPlayer::isPaused()
{
    if (mState)
        return mState->mPaused;
    return true;
}

double VideoPlayer::getCurrentTime()
{
    if (mState)
        return mState->get_master_clock();
    return 0.0;
}

void VideoPlayer::seek(double time)
{
    if (mState)
        mState->seekTo(time);
}

double VideoPlayer::getDuration()
{
    if (mState)
        return mState->getDuration();
    return 0.0;
}

}

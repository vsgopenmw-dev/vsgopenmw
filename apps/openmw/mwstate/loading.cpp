#include "loading.hpp"

namespace MWState
{
    Loading::~Loading()
    {
        if (mThread)
            mThread->join();
    }

    float Loading::getComplete() const
    {
        std::lock_guard<std::mutex> lk(mMutex);
        return mComplete;
    }

    void Loading::setComplete(float complete)
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mComplete = complete;
    }

    void Loading::advance()
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mComplete += progressStep;
    }

    std::string Loading::getDescription() const
    {
        std::lock_guard<std::mutex> lk(mMutex);
        return mDescription;
    }

    void Loading::setDescription(const std::string &desc)
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mDescription = desc;
    }

    bool Loading::run(float dt)
    {
        if (!mThread)
        {
            mThread = std::thread([this]{ threadEntryPoint(); });
            return true;
        }
        std::lock_guard<std::mutex> lk(mMutex);
        bool done = mThreadDone;
        if (done && !mError.empty())
            throw std::runtime_error(mError);
        return !done;
    }

    void Loading::threadEntryPoint()
    {
        try
        {
            threadLoad();
        }
        catch (std::exception &e)
        {
            mError = e.what();
        }
        std::lock_guard<std::mutex> lk(mMutex);
        mThreadDone = true;
    }

}

#include "updatethreads.hpp"

#include <iostream>

namespace vsgUtil
{
    UpdateThreads::UpdateThreads(uint32_t numThreads, vsg::ref_ptr<vsg::ActivityStatus> in_status)
        : status(in_status)
    {
        if (!status)
            status = vsg::ActivityStatus::create();
        queue = vsg::OperationQueue::create(status);
        auto run = [this]() {
            while (auto operation = queue->take_when_available())
            {
                try
                {
                    operation->run();
                }
                catch (std::exception& e)
                {
                    auto [i, inserted] = loggedErrors.insert(e.what());
                    if (inserted)
                        std::cerr << "!UpdateThreads<" << typeid(*this).name() <<  ">::run(" << e.what() << ")" << std::endl;
                }
                std::unique_lock<std::mutex> lock(mMutex);
                if (--mPending == 0)
                    mCv.notify_all();
            }
            std::unique_lock<std::mutex> lock(mMutex);
            mPending = 0;
            mCv.notify_all();
        };
        for (size_t i = 0; i < numThreads; ++i)
            threads.emplace_back(run);
    }

    void UpdateThreads::waitIdle()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        while (mPending > 0)
            mCv.wait(lock);
    }

    UpdateThreads::~UpdateThreads()
    {
        stop();
    }

    void UpdateThreads::stop()
    {
        status->set(false);
        for (auto& thread : threads)
        {
            thread.join();
        }
        threads.clear();
        mCv.notify_all();
    }

    void UpdateThreads::add(vsg::ref_ptr<vsg::Operation> operation)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        ++mPending;
        queue->add(operation);
    }
}

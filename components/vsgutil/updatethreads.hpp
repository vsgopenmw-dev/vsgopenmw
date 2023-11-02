#ifndef VSGOPENMW_VSGUTIL_UPDATETHREADS_H
#define VSGOPENMW_VSGUTIL_UPDATETHREADS_H

#include <vsg/threading/OperationQueue.h>

#include <set>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace vsgUtil
{
    /*
     * UpdateThreads is a thread pool for distributing update tasks.
     * vsg::OperationThreads is used to implement the threads, with added tracking of the number of pending tasks.
     */
    class UpdateThreads : public vsg::Inherit<vsg::Object, UpdateThreads>
    {
        int mPending = 0;
        std::mutex mMutex;
        std::condition_variable mCv;
    public:
        explicit UpdateThreads(uint32_t numThreads, vsg::ref_ptr<vsg::ActivityStatus> in_status = {});
        UpdateThreads(const UpdateThreads&) = delete;
        UpdateThreads& operator=(const UpdateThreads& rhs) = delete;

        void add(vsg::ref_ptr<vsg::Operation> operation);
        size_t numThreads() const { return threads.size(); }

        /// add multiple objects to the back of the queue
        template<typename Iterator>
        void add(Iterator begin, Iterator end)
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mPending += end-begin;
            queue->add(begin, end);
        }
        template<typename Iterator>
        void run(Iterator begin, Iterator end)
        {
            add(begin, end);
            waitIdle();
        }

        /// stop threads
        void stop();

        void waitIdle();

        std::set<std::string> loggedErrors;

    protected:
        std::list<std::thread> threads;
        vsg::ref_ptr<vsg::OperationQueue> queue;
        vsg::ref_ptr<vsg::ActivityStatus> status;

        virtual ~UpdateThreads();
    };
}

#endif

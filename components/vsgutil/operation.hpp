#ifndef VSGOPENMW_VSGUTIL_OPERATION_H
#define VSGOPENMW_VSGUTIL_OPERATION_H

#include <iostream>
#include <atomic>
#include <optional>
#include <stdexcept>
#include <string>
#include <condition_variable>

#include <vsg/threading/OperationQueue.h>
#include <vsg/core/Exception.h>

namespace vsgUtil
{
    /*
     * Tracks completion.
     */
    struct Operation : public vsg::Operation
    {
        std::atomic_bool done = false;
        virtual void operate() = 0;
        void run() override
        {
            try
            {
                operate();
            }
            catch (std::exception& e)
            {
                mError = e.what();
                std::cerr << "!Operation::run( " << e.what() << ")" << std::endl;
            }
            catch (vsg::Exception& e)
            {
                mError = e.message;
                std::cerr << "!Operation::run(vsg::Exception(" << e.message << "))" << std::endl;
            }
            done = true;
            mCv.notify_all();
        }
        void wait()
        {
            while (!done)
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mCv.wait(lock);
            }
        }
        void ensure()
        {
            wait();
            if (mError)
                throw std::runtime_error(*mError);
        }
    private:
        std::condition_variable mCv;
        std::mutex mMutex;
        std::optional<std::string> mError;
    };
}

#endif

#ifndef VSGOPENMW_RENDER_LIMITFRAMERATE_H
#define VSGOPENMW_RENDER_LIMITFRAMERATE_H

#include <chrono>
#include <thread>

namespace Render
{
    class LimitFramerate
    {
        public:
            template <class Rep, class Ratio>
            explicit LimitFramerate(std::chrono::duration<Rep, Ratio> maxFrameDuration,
                                      std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
                : mMaxFrameDuration(std::chrono::duration_cast<std::chrono::steady_clock::duration>(maxFrameDuration))
                , mLastMeasurement(now)
                , mLastFrameDuration(0)
            {}

            static LimitFramerate create(float frameRateLimit)
            {
                if (frameRateLimit > 0.0f)
                    return LimitFramerate(std::chrono::duration<float>(1.0f / frameRateLimit));
                else
                    return LimitFramerate(std::chrono::steady_clock::duration::zero());
            }

            std::chrono::steady_clock::duration getLastFrameDuration() const
            {
                return mLastFrameDuration;
            }

            void limit(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
            {
                const auto passed = now - mLastMeasurement;
                const auto left = mMaxFrameDuration - passed;
                if (left > left.zero())
                {
                    std::this_thread::sleep_for(left);
                    mLastMeasurement = now + left;
                    mLastFrameDuration = mMaxFrameDuration;
                }
                else
                {
                    mLastMeasurement = now;
                    mLastFrameDuration = passed;
                }
            }

        private:
            std::chrono::steady_clock::duration mMaxFrameDuration;
            std::chrono::steady_clock::time_point mLastMeasurement;
            std::chrono::steady_clock::duration mLastFrameDuration;
    };
}

#endif

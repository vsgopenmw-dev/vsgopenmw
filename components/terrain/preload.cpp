#include "preload.hpp"

#include <components/vsgutil/operation.hpp>
#include <components/vsgutil/compileop.hpp>

#include "paging.hpp"

namespace
{
    template <class Contained>
    bool contains(const std::vector<Terrain::Preload::Position>& container, const Contained& contained, float sqrTolerance)
    {
        for (const auto& pos : contained)
        {
            bool found = false;
            for (const auto& pos2 : container)
            {
                if (vsg::length2(pos.first - pos2.first) < sqrTolerance && pos.second == pos2.second)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return true;
    }
}
namespace Terrain
{
    Preload::Preload(Paging* terrain, vsg::ref_ptr<vsg::OperationThreads> loadThreads, vsg::ref_ptr<vsg::OperationThreads> compileThreads)
        : mTerrain(terrain)
        , mLoadThreads(loadThreads)
        , mCompileThreads(compileThreads)
    {
    }

    Preload::~Preload()
    {
        if (mOperation)
            mOperation->wait();
    }

    void Preload::setPositions(const std::vector<Preload::Position>& positions)
    {
        if (positions.empty())
            mPositions.clear();
        else if (contains(mPositions, positions, View::sqrReuseDistance))
            return;

        if (mOperation && !mOperation->done)
            return;
        else
        {
            auto numViews = positions.size();
            if (mViews.size() > numViews)
                mViews.resize(numViews);
            else if (mViews.size() < numViews)
            {
                for (unsigned int i = mViews.size(); i < numViews; ++i)
                    mViews.emplace_back(mTerrain->createView());
            }

            mPositions = positions;
            if (!positions.empty())
            {
                mOperation = new vsgUtil::OperationFunc( [this, positions]() {
                    for (size_t i = 0; i < positions.size(); ++i)
                    {
                        auto compile = mTerrain->updateView(*mViews[i], positions[i].first, positions[i].second, true);
                        if (compile)
                            compile->compile();
                        //if(lastCompile->done())
                        //    mCompileThreads->add(compile);
                    }
                });
                mLoadThreads->add(mOperation);
            }
        }
    }
}

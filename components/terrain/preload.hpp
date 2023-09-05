#ifndef VSGOPENMW_TERRAIN_PRELOAD_H
#define VSGOPENMW_TERRAIN_PRELOAD_H

#include <memory>

#include <vsg/threading/OperationThreads.h>

namespace vsgUtil
{
    class Operation;
}
namespace Terrain
{
    class Paging;
    class View;

    class Preload
    {
    public:
        Preload(Paging* terrain, vsg::ref_ptr<vsg::OperationThreads> loadThreads, vsg::ref_ptr<vsg::OperationThreads> compileThreads);
        ~Preload();

        using Position = std::pair<vsg::dvec3, float>; // float viewDistance
        void setPositions(const std::vector<Position>& positions);
    private:
        std::vector<std::unique_ptr<View>> mViews;
        std::vector<Position> mPositions;
        vsg::ref_ptr<vsgUtil::Operation> mOperation;

        Paging* mTerrain;
        vsg::ref_ptr<vsg::OperationThreads> mLoadThreads;
        vsg::ref_ptr<vsg::OperationThreads> mCompileThreads;
    };
}

#endif

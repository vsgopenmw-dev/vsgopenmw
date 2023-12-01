#ifndef OPENMW_MWRENDER_LANDMANAGER_H
#define OPENMW_MWRENDER_LANDMANAGER_H


#include <components/esm/util.hpp>
#include <components/esm3terrain/storage.hpp>
#include <components/vsgutil/cache.hpp>

namespace ESM
{
    struct Land;
}
namespace MWRender
{

    class LandManager
    {
    public:
        LandManager(int loadFlags);

        /// @note Will return nullptr if not found.
        vsg::ref_ptr<ESMTerrain::LandObject> getLand(ESM::ExteriorCellLocation cellIndex) const;

        vsg::ref_ptr<ESMTerrain::LandObject> create(ESM::ExteriorCellLocation cellIndex) const;

        void pruneCache() { mCache.prune(); }

    private:
        int mLoadFlags;
        mutable vsgUtil::RefCache<ESM::ExteriorCellLocation, ESMTerrain::LandObject> mCache;
    };
}

#endif

#include "landmanager.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    LandManager::LandManager(int loadFlags)
        : mLoadFlags(loadFlags)
    {
    }

    vsg::ref_ptr<ESMTerrain::LandObject> LandManager::create(ESM::ExteriorCellLocation cellIndex) const
    {
        const ESM::Land* land = MWBase::Environment::get().getESMStore()->get<ESM::Land>().search(cellIndex.mX, cellIndex.mY);
        if (land)
            return vsg::ref_ptr{ new ESMTerrain::LandObject(*land, mLoadFlags) };
        else
            return vsg::ref_ptr{ new ESMTerrain::LandObject() };
    }
 
    vsg::ref_ptr<ESMTerrain::LandObject> LandManager::getLand(ESM::ExteriorCellLocation cellIndex) const
    {
        return mCache.getOrCreate(cellIndex, *this);
    }
}

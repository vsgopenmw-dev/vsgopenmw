#ifndef OPENMW_MWRENDER_LANDMANAGER_H
#define OPENMW_MWRENDER_LANDMANAGER_H


#include <components/esm/util.hpp>
#include <components/esm3terrain/storage.hpp>
#include <components/resource/resourcemanager.hpp>

namespace ESM
{
    struct Land;
}
namespace MWRender
{
<<<<<<< HEAD

    class LandManager : public Resource::GenericResourceManager<ESM::ExteriorCellLocation>
=======
    class LandManager : public Resource::GenericResourceManager<std::pair<int, int>>
>>>>>>> 954897300b (vsgopenmw-openmw)
    {
    public:
        LandManager(int loadFlags);

        /// @note Will return nullptr if not found.
        vsg::ref_ptr<ESMTerrain::LandObject> getLand(ESM::ExteriorCellLocation cellIndex);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        int mLoadFlags;
    };
}

#endif

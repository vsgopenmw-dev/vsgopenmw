// vsgopenmw-unity-build

#include <components/esm3/cellid.hpp>

#include "../mwworld/scene.hpp"

#include "loading.hpp"

namespace MWState
{
    class PreloadCell : public Loading
    {
        MWWorld::Scene& mScene;
    public:
        ESM::CellId cell;
        PreloadCell(MWWorld::Scene& scene, const ESM::CellId& in_cell)
            : mScene(scene)
            , cell(in_cell)
        {
            if (cell.mPaged)
                mDescription = "#{sLoadingMessage3}";
            else
                mDescription = "#{sLoadingMessage2}";
        }
        bool run(float dt) override
        {
            mComplete = mScene.preloadCell(cell, true);
            return mComplete < 1 && !abort;
        }
    };
}

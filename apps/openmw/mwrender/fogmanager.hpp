#ifndef OPENMW_MWRENDER_FOGMANAGER_H
#define OPENMW_MWRENDER_FOGMANAGER_H

#include <vsg/maths/vec4.h>

namespace ESM
{
    struct Cell;
}

namespace MWRender
{
    class FogManager
    {
    public:
        FogManager();

        void configure(float viewDistance, const ESM::Cell *cell);
        void configure(float viewDistance, float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const vsg::vec4 &color);

        vsg::vec4 getFogColor(bool isUnderwater) const;
        float getFogStart(bool isUnderwater) const;
        float getFogEnd(bool isUnderwater) const;

    private:
        float mLandFogStart;
        float mLandFogEnd;
        float mUnderwaterFogStart;
        float mUnderwaterFogEnd;
        vsg::vec4 mFogColor;
        bool mDistantFog;

        vsg::vec4 mUnderwaterColor;
        float mUnderwaterWeight;
        float mUnderwaterIndoorFog;
    };
}

#endif

#ifndef VSGOPENMW_PIPELINE_MODE_H
#define VSGOPENMW_PIPELINE_MODE_H

namespace Pipeline
{
    /*
     * Controls shader modes requiring descriptors.
     */
    enum class Mode
    {
        MATERIAL = 0,
        TEXMAT,
        PARTICLE,
        BILLBOARD,
        DIFFUSE_MAP,
        DARK_MAP,
        DETAIL_MAP,
        DECAL_MAP,
        GLOW_MAP,
        BUMP_MAP,
        ENV_MAP,
        MORPH
    };

    /*
     * Controls shader modes requiring vertex input bindings.
     */
    enum class GeometryMode
    {
        VERTEX = 0,
        NORMAL,
        COLOR,
        SKIN,
        TEXCOORD
    };
}

#endif

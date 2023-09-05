#ifndef VSGOPENMW_PIPELINE_MODE_H
#define VSGOPENMW_PIPELINE_MODE_H

#include <vector>

namespace Pipeline
{
    /*
     * Controls shader variations.
     */
    enum class Mode
    {
        VERTEX,
        NORMAL,
        COLOR,
        SKIN,
        TEXCOORD,
        MATERIAL,
        TEXMAT,
        MORPH,
        PARTICLE,
        BILLBOARD,
        DIFFUSE_MAP,
        DARK_MAP,
        DETAIL_MAP,
        DECAL_MAP,
        GLOW_MAP,
        BUMP_MAP,
        ENV_MAP
    };

    const std::vector<Mode> vertexModes { Mode::VERTEX, Mode::NORMAL, Mode::COLOR, Mode::TEXCOORD, Mode::PARTICLE, Mode::BILLBOARD, Mode::MORPH, Mode::SKIN, Mode::TEXMAT, Mode::ENV_MAP };
    const std::vector<Mode> fragmentModes { Mode::NORMAL, Mode::COLOR, Mode::TEXCOORD, Mode::PARTICLE/*_TEXCOORD*/, Mode::DIFFUSE_MAP, Mode::DARK_MAP, Mode::DETAIL_MAP, Mode::DECAL_MAP, Mode::GLOW_MAP, Mode::BUMP_MAP, Mode::ENV_MAP, Mode::MATERIAL };

    using Modes = std::vector<bool>;

    inline void addMode(Mode mode, Modes& vec)
    {
        auto i = static_cast<size_t>(mode);
        if (i >= vec.size())
            vec.resize(i + 1);
        vec[i] = true;
    }

    inline bool hasMode(Mode mode, const Modes& vec)
    {
        auto i = static_cast<size_t>(mode);
        return i < vec.size() && vec[i];
    }

    inline Modes modes (const std::vector<Mode>& vec)
    {
        Modes ret;
        for (auto mode : vec)
            addMode(mode, ret);
        return ret;
    }
}

#endif

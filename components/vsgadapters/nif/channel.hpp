#ifndef VSGOPENMW_VSGADAPTERS_NIF_CHANNEL_H
#define VSGOPENMW_VSGADAPTERS_NIF_CHANNEL_H

#include <components/animation/channel.hpp>
#include <components/nif/data.hpp>

namespace vsgAdapters
{
    struct FlipChannel : public Anim::Channel<size_t>
    {
        FlipChannel(float d, size_t n)
            : delta(d)
            , numTextures(n)
        {
        }
        size_t value(float time) const override { return static_cast<size_t>(time / delta) % numTextures; }
        float delta;
        size_t numTextures;
    };

    struct VisChannel : public Anim::Channel<bool>
    {
        VisChannel(const std::vector<Nif::NiVisData::VisData>& d)
            : data(d)
        {
        }
        std::vector<Nif::NiVisData::VisData> data;
        bool value(float time) const override
        {
            for (size_t i = 1; i < data.size(); ++i)
            {
                if (data[i].time > time)
                    return data[i - 1].isSet;
            }
            return data.back().isSet;
        }
    };
}

#endif

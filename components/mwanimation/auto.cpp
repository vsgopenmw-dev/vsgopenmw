#include "auto.hpp"

namespace MWAnim
{
    Auto::Auto(const Context& ctx)
        : Object(ctx)
    {
    }

    void Auto::update(float dt)
    {
        autoPlay.update(dt);
    }
}

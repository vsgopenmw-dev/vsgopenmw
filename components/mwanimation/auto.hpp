#ifndef VSGOPENMW_MWANIMATION_AUTO_H
#define VSGOPENMW_MWANIMATION_AUTO_H

#include "object.hpp"

namespace MWAnim
{
    class Context;

    /*
     * Creates very simple animation scene graph.
     */
    class Auto : public Object
    {
    public:
        Auto(const Context& ctx);
        // this->animation = delete;
        void update(float dt) override;
    };
}

#endif

#ifndef VSGOPENMW_MWANIMATION_AUTO_H
#define VSGOPENMW_MWANIMATION_AUTO_H

#include "object.hpp"

namespace ESM
{
    class Light;
}
namespace MWAnim
{
    class Context;

    /*
     * Creates very simple animation scene graph.
     */
    class Auto : public Object
    {
    public:
        Auto(const Context &ctx);
        Auto(const Context &ctx, const ESM::Light &light);
        Auto(const Context &ctx, const std::string &model, const ESM::Light *light);
        //this->animation = delete;
        void update(float dt) override;
    };
}

#endif

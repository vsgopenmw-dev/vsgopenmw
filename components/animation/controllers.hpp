#ifndef VSGOPENMW_ANIMATION_CONTROLLERS_H
#define VSGOPENMW_ANIMATION_CONTROLLERS_H

#include <vector>

namespace vsg
{
    class Object;
}
namespace Anim
{
    class Controller;

    /*
     * Targets controllers.
     */
    using Controllers = std::vector<std::pair<const Controller*, vsg::Object*>>;
}

#endif

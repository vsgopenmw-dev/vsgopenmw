#ifndef VSGOPENMW_ANIMATION_CONTROLLERMAP_H
#define VSGOPENMW_ANIMATION_CONTROLLERMAP_H

#include <unordered_map>

#include "transformcontroller.hpp"
#include "tags.hpp"

namespace Anim
{
    /*
     * Maps detached controllers by target name.
     */
    class ControllerMap : public vsg::Object
    {
    public:
        std::unordered_map<std::string, vsg::ref_ptr<const TransformController>> map;
        vsg::ref_ptr<Anim::Tags> tags;
    };
}

#endif

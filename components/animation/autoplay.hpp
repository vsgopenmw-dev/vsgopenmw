#ifndef VSGOPENMW_ANIMATION_AUTOPLAY_H
#define VSGOPENMW_ANIMATION_AUTOPLAY_H

#include "update.hpp"

namespace Anim
{
    /*
     * Supports random phase.
     */
    struct AutoPlay
    {
        struct Group : public AutoUpdate
        {
            int phaseGroup = 0; // = Group_NotRandom;
        };
        std::vector<Group> groups;
        void update(float dt)
        {
            for (auto& group : groups)
                group.update(dt);
        }
        int maxPhaseGroup() const
        {
            int phaseGroup = 0;
            for (auto& group : groups)
                phaseGroup = std::max(group.phaseGroup, phaseGroup);
            return phaseGroup;
        }
        /*
         * @param phaseGroupOffset Signifies that controller belongs to another file with distinct groups.
         */
        void add(const Controller* ctrl, vsg::Object* target, int phaseGroupOffset=0)
        {
            int phaseGroup = ctrl->hints.phaseGroup;
            if (phaseGroup != 0)
                phaseGroup += phaseGroupOffset;
            auto& group = getOrCreateGroup(phaseGroup);
            group.controllers.emplace_back(ctrl, target);
        }
        Group& getOrCreateGroup(int phaseGroup=0)
        {
            for (auto& i : groups)
            {
                if (i.phaseGroup == phaseGroup)
                    return i;
            }
            auto& group = groups.emplace_back();
            group.phaseGroup = phaseGroup;
            if (phaseGroup != 0)
                group.timer = std::rand() / static_cast<float>(RAND_MAX);
            return group;
        }
        void clear()
        {
            groups.clear();
        }
    };
}

#endif

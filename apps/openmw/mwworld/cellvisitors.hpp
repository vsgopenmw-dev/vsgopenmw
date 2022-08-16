#ifndef GAME_MWWORLD_CELLVISITORS_H
#define GAME_MWWORLD_CELLVISITORS_H

#include <vector>

#include "ptr.hpp"


namespace MWWorld
{
    struct ListAndResetObjectsVisitor
    {
        std::vector<MWWorld::Ptr> mObjects;

        bool operator() (const MWWorld::Ptr& ptr)
        {
            mObjects.push_back (ptr);

            return true;
        }
    };

}

#endif

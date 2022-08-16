#ifndef OPENMW_MWCLASS_CLASSMODEL_H
#define OPENMW_MWCLASS_CLASSMODEL_H

#include "../mwworld/ptr.hpp"
#include "../mwworld/livecellref.hpp"

#include <string>

namespace MWClass
{
    template <class Class>
    std::string getClassModel(const MWWorld::ConstPtr& ptr)
    {
        return ptr.get<Class>()->mBase->mModel;
    }
}

#endif

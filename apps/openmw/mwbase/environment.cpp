#include "environment.hpp"

#include <cassert>

// TODO: shouldn't we deprecate the NotNullPtr/pointer get methods, and use references?
// TODO: shouldn't we be creating the Environment object in game.hpp when all its constituents have been created?

MWBase::Environment* MWBase::Environment::sThis = nullptr;

MWBase::Environment::Environment()
{
    assert(sThis == nullptr);
    sThis = this;
}

MWBase::Environment::~Environment()
{
    sThis = nullptr;
}

#include "environment.hpp"

#include <cassert>

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

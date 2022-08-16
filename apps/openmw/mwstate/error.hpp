#ifndef VSGOPENMW_MWSTATE_ERROR_H
#define VSGOPENMW_MWSTATE_ERROR_H

#include <string>

#include "menu.hpp"

namespace MWState
{
    /*
     * Verbosely pauses game.
     */
    class Error : public Menu
    {
        std::string mMessage;
    public:
        Error(const std::string &msg);
        bool run(float dt) override;
    };
}

#endif

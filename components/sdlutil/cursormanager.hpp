#ifndef VSGOPENMW_SDLUTIL_CURSORMANAGER_H
#define VSGOPENMW_SDLUTIL_CURSORMANAGER_H

#include <map>
#include <string>

#include <SDL_types.h>

struct SDL_Cursor;
struct SDL_Surface;

namespace SDLUtil
{
    class CursorManager
    {
    public:
        ~CursorManager();

        /// \brief Tell the manager that the cursor has changed, giving the
        ///        name of the cursor we changed to ("arrow", "ibeam", etc)
        void cursorChanged(const std::string &name);

        void createCursor(const std::string &name, int rotDegrees, SDL_Surface *surface, Uint8 hotspot_x, Uint8 hotspot_y);

    private:
        using CursorMap = std::map<std::string, SDL_Cursor*>;
        CursorMap mCursorMap;

        std::string mCurrentCursor;
    };
}

#endif

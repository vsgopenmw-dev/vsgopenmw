#ifndef VSGOPENMW_SDLUTIL_INPUTWRAPPER_H
#define VSGOPENMW_SDLUTIL_INPUTWRAPPER_H

#include <map>

#include <SDL_events.h>
#include <SDL_version.h>

#include "events.hpp"

namespace SDLUtil
{
    /// \brief A wrapper around SDL's event queue, mostly used for handling input-related events.
    class InputWrapper
    {
    public:
        InputWrapper(SDL_Window *window, bool grab);
        ~InputWrapper();

        void setMouseEventCallback(MouseListener* listen) { mMouseListener = listen; }
        void setSensorEventCallback(SensorListener* listen) { mSensorListener = listen; }
        void setKeyboardEventCallback(KeyListener* listen) { mKeyboardListener = listen; }
        void setWindowEventCallback(WindowListener* listen) { mWindowListener = listen; }
        void setControllerEventCallback(ControllerListener* listen) { mConListener = listen; }

        void capture(bool windowEventsOnly);
        bool isModifierHeld(int mod);
        bool isKeyDown(SDL_Scancode key);

        void setMouseVisible (bool visible);
        /// \brief Set the mouse to relative positioning. Doesn't move the cursor and disables mouse acceleration.
        void setMouseRelative(bool relative);
        bool getMouseRelative() { return mMouseRelative; }

        /// \brief Locks the pointer to the window
        void setGrabPointer(bool grab);

        /// \brief Moves the mouse to the specified point within the viewport
        void warpMouse(int x, int y);

        void updateMouseSettings();

    private:
        void handleWindowEvent(const SDL_Event& evt);
        void windowClosed();

        /// \brief Internal method for ignoring relative motions as a side effect of warpMouse()
        bool _handleWarpMotion(const SDL_MouseMotionEvent& evt);
        /// \brief Wrap the mouse to the viewport
        void _wrapMousePointer(const SDL_MouseMotionEvent &evt);
        /// \brief Package mouse and mousewheel motions into a single event
        MouseMotionEvent _packageMouseMotion(const SDL_Event& evt);

        SDL_Window* mSDLWindow;

        MouseListener* mMouseListener{};
        SensorListener* mSensorListener{};
        KeyListener* mKeyboardListener{};
        WindowListener* mWindowListener{};
        ControllerListener* mConListener{};

        Uint16 mWarpX = 0;
        Uint16 mWarpY = 0;
        bool mWarpCompensate = false;
        bool mWrapPointer = false;

        bool mAllowGrab;
        bool mWantMouseVisible = false;
        bool mWantGrab = false;
        bool mWantRelative = false;
        bool mGrabPointer = false;
        bool mMouseRelative = false;

        bool mFirstMouseMove = true;

        Sint32 mMouseZ = 0;
        Sint32 mMouseX = -1;
        Sint32 mMouseY = -1;

        bool mWindowHasFocus = true;
        bool mMouseInWindow = true;
    };
}

#endif

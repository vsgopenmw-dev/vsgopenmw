#include "inputwrapper.hpp"

#include <iostream>

namespace SDLUtil
{
    InputWrapper::InputWrapper(SDL_Window* window, bool grab) :
        mSDLWindow(window),
        mAllowGrab(grab)
    {
        Uint32 flags = SDL_GetWindowFlags(mSDLWindow);
        mWindowHasFocus = (flags & SDL_WINDOW_INPUT_FOCUS);
        mMouseInWindow = (flags & SDL_WINDOW_MOUSE_FOCUS);
    }

    InputWrapper::~InputWrapper()
    {
    }

    void InputWrapper::capture(bool windowEventsOnly)
    {
        SDL_PumpEvents();

        SDL_Event evt;

        if (windowEventsOnly)
        {
            // During loading, handle window events, discard button presses and keep others for later
            while (SDL_PeepEvents(&evt, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT) > 0)
                handleWindowEvent(evt);
            while (SDL_PeepEvents(&evt, 1, SDL_GETEVENT, SDL_QUIT, SDL_QUIT) > 0)
                windowClosed();

            SDL_FlushEvent(SDL_KEYDOWN);
            SDL_FlushEvent(SDL_CONTROLLERBUTTONDOWN);
            SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
            SDL_FlushEvent(SDL_MOUSEMOTION);
            SDL_FlushEvent(SDL_MOUSEWHEEL);
            return;
        }

        while(SDL_PollEvent(&evt))
        {
            switch(evt.type)
            {
                case SDL_MOUSEMOTION:
                    // Ignore this if it happened due to a warp
                    if(!_handleWarpMotion(evt.motion))
                    {
                        // If in relative mode, don't trigger events unless window has focus
                        if (!mWantRelative || mWindowHasFocus)
                            mMouseListener->mouseMoved(_packageMouseMotion(evt));

                        // Try to keep the mouse inside the window
                        if (mWindowHasFocus)
                            _wrapMousePointer(evt.motion);
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    mMouseListener->mouseMoved(_packageMouseMotion(evt));
                    mMouseListener->mouseWheelMoved(evt.wheel);
                    break;
                case SDL_SENSORUPDATE:
                    mSensorListener->sensorUpdated(evt.sensor);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    mMouseListener->mousePressed(evt.button, evt.button.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    mMouseListener->mouseReleased(evt.button, evt.button.button);
                    break;
                case SDL_KEYDOWN:
                    mKeyboardListener->keyPressed(evt.key);
                    break;
                case SDL_KEYUP:
                    if (!evt.key.repeat)
                        mKeyboardListener->keyReleased(evt.key);
                    break;
                case SDL_TEXTEDITING:
                    break;
                case SDL_TEXTINPUT:
                    mKeyboardListener->textInput(evt.text);
                    break;
                case SDL_KEYMAPCHANGED:
                    break;
                case SDL_JOYHATMOTION: //As we manage everything with GameController, don't even bother with these.
                case SDL_JOYAXISMOTION:
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                case SDL_JOYDEVICEADDED:
                case SDL_JOYDEVICEREMOVED:
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    if(mConListener)
                        mConListener->controllerAdded(1, evt.cdevice); //We only support one joystick, so give everything a generic deviceID
                    break;
                case SDL_CONTROLLERDEVICEREMOVED:
                    if(mConListener)
                        mConListener->controllerRemoved(evt.cdevice);
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    if(mConListener)
                        mConListener->buttonPressed(1, evt.cbutton);
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    if(mConListener)
                        mConListener->buttonReleased(1, evt.cbutton);
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    if(mConListener)
                        mConListener->axisMoved(1, evt.caxis);
                    break;
                #if SDL_VERSION_ATLEAST(2, 0, 14)
                case SDL_CONTROLLERSENSORUPDATE:
                    // controller sensor data is received on demand
                    break;
                case SDL_CONTROLLERTOUCHPADDOWN:
                    mConListener->touchpadPressed(1, TouchEvent(evt.ctouchpad));
                    break;
                case SDL_CONTROLLERTOUCHPADMOTION:
                    mConListener->touchpadMoved(1, TouchEvent(evt.ctouchpad));
                    break;
                case SDL_CONTROLLERTOUCHPADUP:
                    mConListener->touchpadReleased(1, TouchEvent(evt.ctouchpad));
                    break;
                #endif
                case SDL_WINDOWEVENT:
                    handleWindowEvent(evt);
                    break;
                case SDL_QUIT:
                    windowClosed();
                    break;
                case SDL_DISPLAYEVENT:
                    switch (evt.display.event)
                    {
                        case SDL_DISPLAYEVENT_ORIENTATION:
                            if (mSensorListener)
                                mSensorListener->displayOrientationChanged();
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_CLIPBOARDUPDATE:
                    break; // We don't need this event, clipboard is retrieved on demand

                case SDL_FINGERDOWN:
                {
                    int w,h;
                    SDL_GetWindowSize(mSDLWindow, &w, &h);
                    MouseMotionEvent pack_evt{};
                    pack_evt.x = evt.tfinger.x * w;
                    pack_evt.y = evt.tfinger.y * h;
                    pack_evt.z = mMouseZ;
                    mMouseX = pack_evt.x;
                    mMouseY = pack_evt.y;
                    mMouseListener->mouseMoved(pack_evt);
                    evt.button.button = SDL_BUTTON_LEFT;
                    evt.button.x = evt.tfinger.x * w;
                    evt.button.y = evt.tfinger.y * h;
                    mMouseListener->mousePressed(evt.button, evt.button.button);
                    break;
                }
                case SDL_FINGERUP:
                {
                    int w,h;
                    SDL_GetWindowSize(mSDLWindow, &w, &h);
                    MouseMotionEvent pack_evt{};
                    pack_evt.x = evt.tfinger.x * w;
                    pack_evt.y = evt.tfinger.y * h;
                    pack_evt.z = mMouseZ;
                    mMouseX = pack_evt.x;
                    mMouseY = pack_evt.y;
                    mMouseListener->mouseMoved(pack_evt);
                    evt.button.button = SDL_BUTTON_LEFT;
                    evt.button.x = evt.tfinger.x *w;
                    evt.button.y = evt.tfinger.y *h;
                    mMouseListener->mouseReleased(evt.button, evt.button.button);
                    break;
                }
                case SDL_FINGERMOTION:
                {
                    int w,h;
                    SDL_GetWindowSize(mSDLWindow, &w, &h);
                    MouseMotionEvent pack_evt{};
                    pack_evt.x = evt.tfinger.x * w;
                    pack_evt.y = evt.tfinger.y * h;
                    pack_evt.xrel = pack_evt.x - mMouseX;
                    pack_evt.yrel = pack_evt.y - mMouseY;
                    pack_evt.z =  mMouseZ;
                    mMouseX = pack_evt.x;
                    mMouseY = pack_evt.y;
                    mMouseListener->mouseMoved(pack_evt);
                    break;
                }
                case SDL_DOLLARGESTURE:
                case SDL_DOLLARRECORD:
                case SDL_MULTIGESTURE:
                    // No use for touch & gesture events
                    break;

                case SDL_APP_WILLENTERBACKGROUND:
                case SDL_APP_WILLENTERFOREGROUND:
                case SDL_APP_DIDENTERBACKGROUND:
                case SDL_APP_DIDENTERFOREGROUND:
                    // We do not need background/foreground switch event for mobile devices so far
                    break;

                case SDL_APP_TERMINATING:
                    // There is nothing we can do here.
                    break;

                case SDL_APP_LOWMEMORY:
                    std::cerr << "System reports that free RAM on device is running low." << std::endl;
                    break;

                default:
                    std::cerr << "Unhandled SDL event of type 0x" << std::hex << evt.type << std::endl;
                    break;
            }
        }
    }

    void InputWrapper::handleWindowEvent(const SDL_Event& evt)
    {
        switch (evt.window.event) {
            case SDL_WINDOWEVENT_ENTER:
                mMouseInWindow = true;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_LEAVE:
                mMouseInWindow = false;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_MOVED:
                break;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                int w,h;
                SDL_GetWindowSize(mSDLWindow, &w, &h);
                int x,y;
                SDL_GetWindowPosition(mSDLWindow, &x,&y);

                if (mWindowListener)
                    mWindowListener->windowResized(w, h);

                break;

            case SDL_WINDOWEVENT_RESIZED:
                // This should also fire SIZE_CHANGED, so no need to handle
                break;

            case SDL_WINDOWEVENT_FOCUS_GAINED:
                mWindowHasFocus = true;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                mWindowHasFocus = false;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_CLOSE:
                break;
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_RESTORED:
                if (mWindowListener)
                    mWindowListener->windowVisibilityChange(true);
                break;
            case SDL_WINDOWEVENT_HIDDEN:
            case SDL_WINDOWEVENT_MINIMIZED:
                if (mWindowListener)
                    mWindowListener->windowVisibilityChange(false);
                break;
        }
    }

    void InputWrapper::windowClosed()
    {
        if (mWindowListener)
            mWindowListener->windowClosed();
    }

    bool InputWrapper::isModifierHeld(int mod)
    {
        return (SDL_GetModState() & mod) != 0;
    }

    bool InputWrapper::isKeyDown(SDL_Scancode key)
    {
        return (SDL_GetKeyboardState(nullptr)[key]) != 0;
    }

    void InputWrapper::warpMouse(int x, int y)
    {
        SDL_WarpMouseInWindow(mSDLWindow, x, y);
        mWarpCompensate = true;
        mWarpX = x;
        mWarpY = y;
    }

    void InputWrapper::setGrabPointer(bool grab)
    {
        mWantGrab = grab;
        updateMouseSettings();
    }

    void InputWrapper::setMouseRelative(bool relative)
    {
        mWantRelative = relative;
        updateMouseSettings();
    }

    void InputWrapper::setMouseVisible(bool visible)
    {
        mWantMouseVisible = visible;
        updateMouseSettings();
    }

    void InputWrapper::updateMouseSettings()
    {
        mGrabPointer = mWantGrab && mMouseInWindow && mWindowHasFocus;
        SDL_SetWindowGrab(mSDLWindow, mGrabPointer && mAllowGrab ? SDL_TRUE : SDL_FALSE);

        SDL_ShowCursor(mWantMouseVisible || !mWindowHasFocus);

        bool relative = mWantRelative && mMouseInWindow && mWindowHasFocus;
        if(mMouseRelative == relative)
            return;

        mMouseRelative = relative;

        mWrapPointer = false;

        // eep, wrap the pointer manually if the input driver doesn't support
        // relative positioning natively
        // also use wrapping if no-grab was specified in options (SDL_SetRelativeMouseMode
        // appears to eat the mouse cursor when pausing in a debugger)
        bool success = mAllowGrab && SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == 0;
        if(relative && !success)
            mWrapPointer = true;

        //now remove all mouse events using the old setting from the queue
        SDL_PumpEvents();
        SDL_FlushEvent(SDL_MOUSEMOTION);
    }

    bool InputWrapper::_handleWarpMotion(const SDL_MouseMotionEvent& evt)
    {
        if(!mWarpCompensate)
            return false;

        //this was a warp event, signal the caller to eat it.
        if(evt.x == mWarpX && evt.y == mWarpY)
        {
            mWarpCompensate = false;
            return true;
        }

        return false;
    }

    void InputWrapper::_wrapMousePointer(const SDL_MouseMotionEvent& evt)
    {
        //don't wrap if we don't want relative movements, support relative
        //movements natively, or aren't grabbing anyways
        if(!mMouseRelative || !mWrapPointer || !mGrabPointer)
            return;

        int width = 0;
        int height = 0;

        SDL_GetWindowSize(mSDLWindow, &width, &height);

        const int FUDGE_FACTOR_X = width/4;
        const int FUDGE_FACTOR_Y = height/4;

        //warp the mouse if it's about to go outside the window
        if(evt.x - FUDGE_FACTOR_X < 0  || evt.x + FUDGE_FACTOR_X > width
                || evt.y - FUDGE_FACTOR_Y < 0 || evt.y + FUDGE_FACTOR_Y > height)
        {
            warpMouse(width / 2, height / 2);
        }
    }

    MouseMotionEvent InputWrapper::_packageMouseMotion(const SDL_Event &evt)
    {
        MouseMotionEvent pack_evt{};
        pack_evt.x = mMouseX;
        pack_evt.y = mMouseY;
        pack_evt.z = mMouseZ;

        if(evt.type == SDL_MOUSEMOTION)
        {
            pack_evt.x = mMouseX = evt.motion.x;
            pack_evt.y = mMouseY = evt.motion.y;
            pack_evt.xrel = evt.motion.xrel;
            pack_evt.yrel = evt.motion.yrel;
            pack_evt.type = SDL_MOUSEMOTION;
            if (mFirstMouseMove)
            {
                // first event should be treated as non-relative, since there's no point of reference
                // SDL then (incorrectly) uses (0,0) as point of reference, on Linux at least...
                pack_evt.xrel = pack_evt.yrel = 0;
                mFirstMouseMove = false;
            }
        }
        else if(evt.type == SDL_MOUSEWHEEL)
        {
            mMouseZ += pack_evt.zrel = (evt.wheel.y * 120);
            pack_evt.z = mMouseZ;
            pack_evt.type = SDL_MOUSEWHEEL;
        }
        else
        {
            throw std::runtime_error("Tried to package non-motion event!");
        }

        return pack_evt;
    }
}

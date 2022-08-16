#ifndef MWINPUT_ACTIONMANAGER_H
#define MWINPUT_ACTIONMANAGER_H

namespace MWInput
{
    class BindingsManager;

    class ActionManager
    {
    public:

        ActionManager(BindingsManager* bindingsManager);

        void update(float dt, bool triedToMove);

        void executeAction(int action);

        bool checkAllowedToUseItems() const;

        void toggleMainMenu();
        void toggleSpell();
        void toggleWeapon();
        void toggleInventory();
        void toggleConsole();
        void screenshot();
        void toggleJournal();
        void activate();
        void toggleWalking();
        void toggleSneaking();
        void toggleAutoMove();
        void rest();
        void quickLoad();
        void quickSave();

        void quickKey (int index);
        void showQuickKeysMenu();

        void resetIdleTime();
        float getIdleTime() const { return mTimeIdle; }

        bool isAlwaysRunActive() const { return mAlwaysRunActive; };
        bool isSneaking() const { return mSneaking; };

        void setAttemptJump(bool enabled) { mAttemptJump = enabled; }

        bool screenshotRequest{};

    private:
        void handleGuiArrowKey(int action);

        BindingsManager* mBindingsManager;

        bool mAlwaysRunActive;
        bool mSneaking;
        bool mAttemptJump;

        float mTimeIdle;
    };
}
#endif

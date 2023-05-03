#ifndef MWINPUT_ACTIONMANAGER_H
#define MWINPUT_ACTIONMANAGER_H

namespace MWInput
{
    class BindingsManager;

    class ActionManager
    {
    public:
        ActionManager(BindingsManager* bindingsManager);

        void update(float dt);

        void executeAction(int action);

        bool checkAllowedToUseItems() const;

        void toggleMainMenu();
        void toggleInventory();
        void toggleConsole();
        void screenshot();
        void toggleJournal();
        void activate();
        void rest();
        void quickLoad();
        void quickSave();

        void quickKey(int index);
        void showQuickKeysMenu();

        void resetIdleTime();
        float getIdleTime() const { return mTimeIdle; }

        bool isSneaking() const;

        bool screenshotRequest{};

    private:
        void handleGuiArrowKey(int action);

        BindingsManager* mBindingsManager;

        float mTimeIdle;
    };
}
#endif

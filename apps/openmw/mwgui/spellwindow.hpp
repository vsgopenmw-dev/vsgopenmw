#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include <memory>

#include "windowpinnablebase.hpp"

#include "spellmodel.hpp"

namespace MWGui
{
    class SpellIcons;
    class SpellView;

    class SpellWindow : public WindowPinnableBase, public NoDrop
    {
    public:
        SpellWindow(DragAndDrop* drag);
        ~SpellWindow();

        void updateSpells();

        void onFrame(float dt) override;

        /// Cycle to next/previous spell
        void cycle(bool next);

    protected:
        MyGUI::Widget* mEffectBox;

        ESM::RefId mSpellToDelete;

        void onEnchantedItemSelected(MWWorld::Ptr item, bool alreadyEquipped);
        void onSpellSelected(const ESM::RefId& spellId);
        void onModelIndexSelected(SpellModel::ModelIndex index);
        void onFilterChanged(MyGUI::EditBox* sender);
        void onDeleteClicked(MyGUI::Widget* widget);
        void onDeleteSpellAccept();
        void askDeleteSpell(const ESM::RefId& spellId);

        void onPinToggled() override;
        void onTitleDoubleClicked() override;
        void onOpen() override;

        SpellView* mSpellView;
        std::unique_ptr<SpellIcons> mSpellIcons;
        MyGUI::EditBox* mFilterEdit;

    private:
        float mUpdateTimer;
    };
}

#endif

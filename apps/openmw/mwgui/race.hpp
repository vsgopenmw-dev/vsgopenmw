#ifndef MWGUI_RACE_H
#define MWGUI_RACE_H

#include "windowbase.hpp"
#include <components/esm/refid.hpp>
#include <memory>

#include <components/esm3/loadnpc.hpp>

namespace MWRender
{
    class Preview;
    class RaceSelection;
}

namespace MWGui
{
    class RaceDialog : public WindowModal
    {
    public:
        RaceDialog(MWRender::Preview* p);
        ~RaceDialog();

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const ESM::NPC& getResult() const { return mProto; }
        const ESM::RefId& getRaceId() const { return mProto.mRace; }
        Gender getGender() const { return mGenderIndex == 0 ? GM_Male : GM_Female; }

        void setRaceId(const ESM::RefId& raceId);
        void setGender(Gender gender) { mGenderIndex = gender == GM_Male ? 0 : 1; }

        void setNextButtonShow(bool shown);
        void onOpen() override;
        void onClose() override;

        bool exit() override { return false; }

        // Events
        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;

    protected:
        void onPreviewScroll(MyGUI::Widget* _sender, int _delta);
        void onHeadRotate(MyGUI::ScrollBar* _sender, size_t _position);

        void onSelectPreviousGender(MyGUI::Widget* _sender);
        void onSelectNextGender(MyGUI::Widget* _sender);

        void onSelectPreviousFace(MyGUI::Widget* _sender);
        void onSelectNextFace(MyGUI::Widget* _sender);

        void onSelectPreviousHair(MyGUI::Widget* _sender);
        void onSelectNextHair(MyGUI::Widget* _sender);

        void onSelectRace(MyGUI::ListBox* _sender, size_t _index);
        void onAccept(MyGUI::ListBox* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateRaces();
        void updateSkills();
        void updateSpellPowers();
        void updatePreview();
        void recountParts();

        void getBodyParts(int part, std::vector<ESM::RefId>& out);

        std::vector<ESM::RefId> mAvailableHeads;
        std::vector<ESM::RefId> mAvailableHairs;

        MyGUI::ImageBox* mPreviewImage;
        MyGUI::ListBox* mRaceList;
        MyGUI::ScrollBar* mHeadRotate;

        MyGUI::Widget* mSkillList;
        std::vector<MyGUI::Widget*> mSkillItems;

        MyGUI::Widget* mSpellPowerList;
        std::vector<MyGUI::Widget*> mSpellPowerItems;

        int mGenderIndex, mFaceIndex, mHairIndex;

        float mCurrentAngle;

        std::unique_ptr<MWRender::RaceSelection> mPreview;
        std::unique_ptr<MyGUI::ITexture> mPreviewTexture;
        ESM::NPC mProto;
    };
}
#endif

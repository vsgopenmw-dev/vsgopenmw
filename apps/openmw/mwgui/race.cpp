#include "race.hpp"

#include <MyGUI_ListBox.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ScrollBar.h>

#include <components/vsgadapters/mygui/texture.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwrender/preview.hpp"

#include "tooltips.hpp"

namespace
{
    int wrap(int index, int max)
    {
        if (index < 0)
            return max - 1;
        else if (index >= max)
            return 0;
        else
            return index;
    }

    bool sortRaces(const std::pair<std::string, std::string>& left, const std::pair<std::string, std::string>& right)
    {
        return left.second.compare(right.second) < 0;
    }
}

namespace MWGui
{
    RaceDialog::RaceDialog(MWRender::Preview *preview)
      : WindowModal("openmw_chargen_race.layout")
      , mGenderIndex(0)
      , mFaceIndex(0)
      , mHairIndex(0)
      , mCurrentAngle(0)
    {
        mPreview = std::make_unique<MWRender::RaceSelection>(preview);
        mPreviewTexture = std::make_unique<vsgAdapters::mygui::Texture>(preview->getTexture());
        mPreviewTexture->setShader("premult_alpha");

        // Centre dialog
        center();

        setText("AppearanceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu1", "Appearance"));
        getWidget(mPreviewImage, "PreviewImage");
        mPreviewImage->setRenderItemTexture(mPreviewTexture.get());

        mPreviewImage->eventMouseWheel += MyGUI::newDelegate(this, &RaceDialog::onPreviewScroll);

        getWidget(mHeadRotate, "HeadRotate");

        mHeadRotate->setScrollRange(1000);
        mHeadRotate->setScrollPosition(500);
        mHeadRotate->setScrollViewPage(50);
        mHeadRotate->setScrollPage(50);
        mHeadRotate->setScrollWheelPage(50);
        mHeadRotate->eventScrollChangePosition += MyGUI::newDelegate(this, &RaceDialog::onHeadRotate);

        // Set up next/previous buttons
        MyGUI::Button *prevButton, *nextButton;

        setText("GenderChoiceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu2", "Change Sex"));
        getWidget(prevButton, "PrevGenderButton");
        getWidget(nextButton, "NextGenderButton");
        prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousGender);
        nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextGender);

        setText("FaceChoiceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu3", "Change Face"));
        getWidget(prevButton, "PrevFaceButton");
        getWidget(nextButton, "NextFaceButton");
        prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousFace);
        nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextFace);

        setText("HairChoiceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu4", "Change Hair"));
        getWidget(prevButton, "PrevHairButton");
        getWidget(nextButton, "NextHairButton");
        prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousHair);
        nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextHair);

        setText("RaceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu5", "Race"));
        getWidget(mRaceList, "RaceList");
        mRaceList->setScrollVisible(true);
        mRaceList->eventListSelectAccept += MyGUI::newDelegate(this, &RaceDialog::onAccept);
        mRaceList->eventListChangePosition += MyGUI::newDelegate(this, &RaceDialog::onSelectRace);

        setText("SkillsT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sBonusSkillTitle", "Skill Bonus"));
        getWidget(mSkillList, "SkillList");
        setText("SpellPowerT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu7", "Specials"));
        getWidget(mSpellPowerList, "SpellPowerList");

        MyGUI::Button* backButton;
        getWidget(backButton, "BackButton");
        backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onBackClicked);

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onOkClicked);

        updateRaces();
        updateSkills();
        updateSpellPowers();
    }

    RaceDialog::~RaceDialog()
    {
    }

    void RaceDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", ""));
        else
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
    }

    void RaceDialog::onOpen()
    {
        WindowModal::onOpen();

        updateRaces();
        updateSkills();
        updateSpellPowers();

        mProto = *MWBase::Environment::get().getWorld()->getPlayerPtr().get<ESM::NPC>()->mBase;
        setRaceId(mProto.mRace);
        setGender(mProto.isMale() ? GM_Male : GM_Female);
        recountParts();

        for (unsigned int i=0; i<mAvailableHeads.size(); ++i)
        {
            if (Misc::StringUtils::ciEqual(mAvailableHeads[i], mProto.mHead))
                mFaceIndex = i;
        }

        for (unsigned int i=0; i<mAvailableHairs.size(); ++i)
        {
            if (Misc::StringUtils::ciEqual(mAvailableHairs[i], mProto.mHair))
                mHairIndex = i;
        }

        updatePreview();
        mPreview->setAngle (mCurrentAngle);

        size_t initialPos = mHeadRotate->getScrollRange()/2+mHeadRotate->getScrollRange()/10;
        mHeadRotate->setScrollPosition(initialPos);
        onHeadRotate(mHeadRotate, initialPos);

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mRaceList);
    }

    void RaceDialog::setRaceId(const std::string &raceId)
    {
        mProto.mRace = raceId;
        mRaceList->setIndexSelected(MyGUI::ITEM_NONE);
        size_t count = mRaceList->getItemCount();
        for (size_t i = 0; i < count; ++i)
        {
            if (Misc::StringUtils::ciEqual(*mRaceList->getItemDataAt<std::string>(i), raceId))
            {
                mRaceList->setIndexSelected(i);
                break;
            }
        }

        updateSkills();
        updateSpellPowers();
    }

    void RaceDialog::onClose()
    {
        WindowModal::onClose();
    }

    // widget controls

    void RaceDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        if(mRaceList->getIndexSelected() == MyGUI::ITEM_NONE)
            return;
        eventDone(this);
    }

    void RaceDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    void RaceDialog::onPreviewScroll(MyGUI::Widget*, int _delta)
    {
        size_t oldPos = mHeadRotate->getScrollPosition();
        size_t maxPos = mHeadRotate->getScrollRange() - 1;
        size_t scrollPage = mHeadRotate->getScrollWheelPage();
        if (_delta < 0)
            mHeadRotate->setScrollPosition(oldPos + std::min(maxPos - oldPos, scrollPage));
        else
            mHeadRotate->setScrollPosition(oldPos - std::min(oldPos, scrollPage));

        onHeadRotate(mHeadRotate, mHeadRotate->getScrollPosition());
    }

    void RaceDialog::onHeadRotate(MyGUI::ScrollBar* scroll, size_t _position)
    {
        float angle = (float(_position) / (scroll->getScrollRange()-1) - 0.5f) * osg::PI * 2;
        mPreview->setAngle (angle);

        mCurrentAngle = angle;
    }

    void RaceDialog::onSelectPreviousGender(MyGUI::Widget*)
    {
        mGenderIndex = wrap(mGenderIndex - 1, 2);

        recountParts();
        updatePreview();
    }

    void RaceDialog::onSelectNextGender(MyGUI::Widget*)
    {
        mGenderIndex = wrap(mGenderIndex + 1, 2);

        recountParts();
        updatePreview();
    }

    void RaceDialog::onSelectPreviousFace(MyGUI::Widget*)
    {
        mFaceIndex = wrap(mFaceIndex - 1, mAvailableHeads.size());
        updatePreview();
    }

    void RaceDialog::onSelectNextFace(MyGUI::Widget*)
    {
        mFaceIndex = wrap(mFaceIndex + 1, mAvailableHeads.size());
        updatePreview();
    }

    void RaceDialog::onSelectPreviousHair(MyGUI::Widget*)
    {
        mHairIndex = wrap(mHairIndex - 1, mAvailableHairs.size());
        updatePreview();
    }

    void RaceDialog::onSelectNextHair(MyGUI::Widget*)
    {
        mHairIndex = wrap(mHairIndex + 1, mAvailableHairs.size());
        updatePreview();
    }

    void RaceDialog::onSelectRace(MyGUI::ListBox* _sender, size_t _index)
    {
        if (_index == MyGUI::ITEM_NONE)
            return;

        const std::string *raceId = mRaceList->getItemDataAt<std::string>(_index);
        if (Misc::StringUtils::ciEqual(mProto.mRace, *raceId))
            return;

        mProto.mRace = *raceId;

        recountParts();

        updatePreview();
        updateSkills();
        updateSpellPowers();
    }

    void RaceDialog::onAccept(MyGUI::ListBox *_sender, size_t _index)
    {
        onSelectRace(_sender, _index);
        if(mRaceList->getIndexSelected() == MyGUI::ITEM_NONE)
            return;
        eventDone(this);
    }

    void RaceDialog::getBodyParts (int part, std::vector<std::string>& out)
    {
        out.clear();
        const MWWorld::Store<ESM::BodyPart> &store =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();

        for (const ESM::BodyPart& bodypart : store)
        {
            if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;
            if (bodypart.mData.mPart != static_cast<ESM::BodyPart::MeshPart>(part))
                continue;
            if (mGenderIndex != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                continue;
            bool firstPerson = (bodypart.mId.size() >= 3)
                    && bodypart.mId[bodypart.mId.size()-3] == '1'
                    && bodypart.mId[bodypart.mId.size()-2] == 's'
                    && bodypart.mId[bodypart.mId.size()-1] == 't';
            if (firstPerson)
                continue;
            if (Misc::StringUtils::ciEqual(bodypart.mRace, mProto.mRace))
                out.push_back(bodypart.mId);
        }
    }

    void RaceDialog::recountParts()
    {
        getBodyParts(ESM::BodyPart::MP_Hair, mAvailableHairs);
        getBodyParts(ESM::BodyPart::MP_Head, mAvailableHeads);

        mFaceIndex = 0;
        mHairIndex = 0;
    }

    // update widget content

    void RaceDialog::updatePreview()
    {
        mProto.setIsMale(mGenderIndex == 0);

        if (mFaceIndex >= 0 && mFaceIndex < int(mAvailableHeads.size()))
            mProto.mHead = mAvailableHeads[mFaceIndex];

        if (mHairIndex >= 0 && mHairIndex < int(mAvailableHairs.size()))
            mProto.mHair = mAvailableHairs[mHairIndex];

        mPreview->setPrototype(mProto);

        //MWGui::PreviewWidget
        auto w = mPreviewImage->getCoord().width;
        auto h = mPreviewImage->getCoord().height;
        auto preview = mPreview->preview;
        preview->setViewport(w, h);
        mPreviewImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, w / float(preview->textureWidth), h / float(preview->textureHeight)));
    }

    void RaceDialog::updateRaces()
    {
        mRaceList->removeAllItems();

        const MWWorld::Store<ESM::Race> &races =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>();

        std::vector<std::pair<std::string, std::string> > items; // ID, name
        for (const ESM::Race& race : races)
        {
            bool playable = race.mData.mFlags & ESM::Race::Playable;
            if (!playable) // Only display playable races
                continue;

            items.emplace_back(race.mId, race.mName);
        }
        std::sort(items.begin(), items.end(), sortRaces);

        int index = 0;
        for (auto& item : items)
        {
            mRaceList->addItem(item.second, item.first);
            if (Misc::StringUtils::ciEqual(item.first, mProto.mRace))
                mRaceList->setIndexSelected(index);
            ++index;
        }
    }

    void RaceDialog::updateSkills()
    {
        for (MyGUI::Widget* widget : mSkillItems)
        {
            MyGUI::Gui::getInstance().destroyWidget(widget);
        }
        mSkillItems.clear();

        if (mProto.mRace.empty())
            return;

        Widgets::MWSkillPtr skillWidget;
        const int lineHeight = 18;
        MyGUI::IntCoord coord1(0, 0, mSkillList->getWidth(), 18);

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Race *race = store.get<ESM::Race>().find(mProto.mRace);
        int count = sizeof(race->mData.mBonus)/sizeof(race->mData.mBonus[0]); // TODO: Find a portable macro for this ARRAYSIZE?
        for (int i = 0; i < count; ++i)
        {
            int skillId = race->mData.mBonus[i].mSkill;
            if (skillId < 0 || skillId > ESM::Skill::Length) // Skip unknown skill indexes
                continue;

            skillWidget = mSkillList->createWidget<Widgets::MWSkill>("MW_StatNameValue", coord1, MyGUI::Align::Default,
                                                           std::string("Skill") + MyGUI::utility::toString(i));
            skillWidget->setSkillNumber(skillId);
            skillWidget->setSkillValue(Widgets::MWSkill::SkillValue(static_cast<float>(race->mData.mBonus[i].mBonus), 0.f));
            ToolTips::createSkillToolTip(skillWidget, skillId);


            mSkillItems.push_back(skillWidget);

            coord1.top += lineHeight;
        }
    }

    void RaceDialog::updateSpellPowers()
    {
        for (MyGUI::Widget* widget : mSpellPowerItems)
        {
            MyGUI::Gui::getInstance().destroyWidget(widget);
        }
        mSpellPowerItems.clear();

        if (mProto.mRace.empty())
            return;

        const int lineHeight = 18;
        MyGUI::IntCoord coord(0, 0, mSpellPowerList->getWidth(), 18);

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Race *race = store.get<ESM::Race>().find(mProto.mRace);

        int i = 0;
        for (const std::string& spellpower : race->mPowers.mList)
        {
            Widgets::MWSpellPtr spellPowerWidget = mSpellPowerList->createWidget<Widgets::MWSpell>("MW_StatName", coord, MyGUI::Align::Default, std::string("SpellPower") + MyGUI::utility::toString(i));
            spellPowerWidget->setSpellId(spellpower);
            spellPowerWidget->setUserString("ToolTipType", "Spell");
            spellPowerWidget->setUserString("Spell", spellpower);

            mSpellPowerItems.push_back(spellPowerWidget);

            coord.top += lineHeight;
            ++i;
        }
    }
}

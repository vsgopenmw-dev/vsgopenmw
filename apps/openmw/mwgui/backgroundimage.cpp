#include "backgroundimage.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_ITexture.h>

namespace MWGui
{

    void BackgroundImage::setBackgroundImage(const std::string& image, bool fixedRatio, bool stretch)
    {
        if (mChild)
        {
            MyGUI::Gui::getInstance().destroyWidget(mChild);
            mChild = nullptr;
        }
        if (!stretch)
        {
            setImageTexture("black");

            if (fixedRatio)
                mAspect = 4.0 / 3.0;
            else
                mAspect = 0; // TODO

            mChild
                = createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(0, 0, 1, 1), MyGUI::Align::Default);
            mChild->setImageTexture(image);

            adjustSize();
        }
        else
        {
            mAspect = 0;
            setImageTexture(image);
        }
    }

    void BackgroundImage::setBackgroundImage(MyGUI::ITexture* tex, bool stretch)
    {
        if (mChild)
        {
            MyGUI::Gui::getInstance().destroyWidget(mChild);
            mChild = nullptr;
        }
        if (!stretch)
        {
            setImageTexture("black");

            mAspect = static_cast<float>(tex->getWidth()) / tex->getHeight();
            mChild
                = createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(0, 0, 1, 1), MyGUI::Align::Default);
            mChild->setRenderItemTexture(tex);
            mChild->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));
            mChild->setNeedKeyFocus(false);
            mChild->setNeedMouseFocus(false);
            adjustSize();
        }
        else
        {
            mAspect = 0;
            setRenderItemTexture(tex);
            getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));
        }
    }

    void BackgroundImage::adjustSize()
    {
        if (mAspect == 0)
            return;

        MyGUI::IntSize screenSize = getSize();

        int leftPadding = std::max(0, static_cast<int>(screenSize.width - screenSize.height * mAspect) / 2);
        int topPadding = std::max(0, static_cast<int>(screenSize.height - screenSize.width / mAspect) / 2);

        mChild->setCoord(
            leftPadding, topPadding, screenSize.width - leftPadding * 2, screenSize.height - topPadding * 2);
    }

    void BackgroundImage::setSize(const MyGUI::IntSize& _value)
    {
        MyGUI::Widget::setSize(_value);
        adjustSize();
    }

    void BackgroundImage::setCoord(const MyGUI::IntCoord& _value)
    {
        MyGUI::Widget::setCoord(_value);
        adjustSize();
    }

}

#ifndef OPENMW_MWGUI_BACKGROUNDIMAGE_H
#define OPENMW_MWGUI_BACKGROUNDIMAGE_H

#include <MyGUI_ImageBox.h>

namespace MWGui
{

    /**
     * @brief A variant of MyGUI::ImageBox with aspect ratio correction using black bars
     */
    class BackgroundImage : public MyGUI::ImageBox
    {
    MYGUI_RTTI_DERIVED(BackgroundImage)

    public:
        /**
         * @param fixedRatio Use a fixed ratio of 4:3, regardless of the image dimensions
         * @param stretch Stretch to fill the whole screen, or add black bars?
         */
        void setBackgroundImage (const std::string& image, bool fixedRatio=true, bool stretch=true);

        void setBackgroundImage (MyGUI::ITexture *tex, bool stretch);

        void setSize (const MyGUI::IntSize &_value) override;
        void setCoord (const MyGUI::IntCoord &_value) override;

    private:
        MyGUI::ImageBox* mChild = nullptr;
        double mAspect = 0.0;

        void adjustSize();
    };

}

#endif

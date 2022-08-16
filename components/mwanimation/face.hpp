#ifndef VSGOPENMW_MWANIMATION_FACE_H
#define VSGOPENMW_MWANIMATION_FACE_H

#include <components/animation/update.hpp>

namespace MWAnim
{
    /*
     * Controls face animations.
     */
    class Face : public Anim::Update
    {
    private:
        float mTalkStart = 0.f;
        float mTalkStop = 0.f;
        float mBlinkStart = 0.f;
        float mBlinkStop = 0.f;
        float mBlinkTimer = 0.f;
        float mValue = 0.f;
        void resetBlinkTimer();
    public:
        Face();
        void loadTags(vsg::Object &node);
        void update(float dt);
        bool enabled = true;
        bool talking = false;
        float loudness = 0.f;
    };
}

#endif

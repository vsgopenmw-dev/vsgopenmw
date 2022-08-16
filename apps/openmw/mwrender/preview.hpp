#ifndef VSGOPENMW_MWRENDER_PREVIEW_H
#define VSGOPENMW_MWRENDER_PREVIEW_H

#include <memory>

#include <vsg/nodes/Switch.h>

#include <components/esm3/loadnpc.hpp>
#include <components/vsgutil/composite.hpp>

#include "../mwworld/ptr.hpp"

namespace vsg
{
    class LookAt;
    class Perspective;
    class Context;
    class CompileManager;
    class ImageView;
}
namespace View
{
    class Descriptors;
}
namespace MWAnim
{
    class Context;
}
namespace Resource
{
    class ResourceSystem;
}
namespace MWRender
{
    class Npc;

    class Preview : public vsgUtil::Composite<vsg::Switch>
    {
        Preview(const Preview&);
        Preview& operator=(const Preview&);
    public:
        Preview(vsg::Context &ctx, vsg::CompileManager *compile, Resource::ResourceSystem* resourceSystem);
        ~Preview();
        void setViewport(uint32_t w, uint32_t h);

        const uint32_t textureWidth = 512;
        const uint32_t textureHeight = 1024;

        void animate();
        void redraw();
        void onFrame();

        void rebuild(const MWWorld::Ptr &, bool headOnly=false);
        void compile();
        void updateViewMatrix(const vsg::dvec3 &eye, const vsg::dvec3 &center);

        vsg::ImageView *getTexture();

        vsg::ref_ptr<vsg::Camera> camera;
        std::unique_ptr<MWRender::Npc> npc;
     protected:
        vsg::ref_ptr<vsg::CompileManager> mCompile;
        vsg::ref_ptr<vsg::Perspective> mPerspective;
        vsg::ref_ptr<vsg::LookAt> mLookAt;
        std::unique_ptr<MWAnim::Context> mContext;
        vsg::vec3 mLightPos;
        vsg::ref_ptr<vsg::Group> mScene;
        std::unique_ptr<View::Descriptors> mDescriptors;
        vsg::ref_ptr<vsg::View> mView;
        vsg::ref_ptr<vsg::ImageView> mTexture;

        bool mNeedRedraw{};
    };

    class Inventory
    {
    public:
        Inventory(Preview *p, const MWWorld::Ptr& character);
        Preview *preview;

        void rebuild(const MWWorld::Ptr& ptr);

        void update(); // Render preview again, e.g. after changed equipment

        int getSlotSelected(int posX, int posY);
    };

    class RaceSelection
    {
        ESM::NPC  mBase;
        MWWorld::LiveCellRef<ESM::NPC> mRef;
        float mPitchRadians;
    public:
        RaceSelection(Preview *p);
        Preview *preview;

        void setAngle(float angleRadians);
        void setPrototype(const ESM::NPC &proto);
    };
}

#endif

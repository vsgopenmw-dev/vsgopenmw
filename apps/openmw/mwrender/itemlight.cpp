#include "itemlight.hpp"

#include <vsg/nodes/Group.h>

#include <components/animation/update.hpp>
#include <components/mwanimation/light.hpp>
#include <components/mwanimation/object.hpp>
#include <components/vsgutil/removechild.hpp>

#include "../mwworld/containerstore.hpp"

namespace
{
    static const std::string sItemLights = "itemlights";
}
namespace MWRender
{
    struct ItemLight
    {
        vsg::ref_ptr<vsg::Node> node;
        Anim::AutoUpdate update;
    };

    using ItemLights = std::map<MWWorld::ConstPtr, ItemLight>;

    class UpdateItemLights : public MWWorld::ContainerStoreListener
    {
    public:
        UpdateItemLights(MWAnim::Object& obj)
            : object(obj)
        {
        }
        MWAnim::Object& object;
        void itemAdded(const MWWorld::ConstPtr& item, int count) override { addItemLightIfRequired(object, item); }
        void itemRemoved(const MWWorld::ConstPtr& item, int count) override { removeItemLight(object, item); }
    };

    void addItemLight(MWAnim::Object& obj, const MWWorld::ConstPtr& item, const ESM::Light& light)
    {
        ItemLights lights;
        obj.node()->getValue(sItemLights, lights);
        if (lights.find(item) != lights.end())
            return;
        // osg::Vec4f ambient(1,1,1,1);
        ItemLight l;
        l.node = MWAnim::addLight(obj.nodeToAddChildrenTo(), obj.nodeToAddChildrenTo(), light, obj.autoPlay.getOrCreateGroup().controllers);
        lights[item] = l;
        obj.node()->setValue(sItemLights, lights);
    }

    void removeItemLight(MWAnim::Object& obj, const MWWorld::ConstPtr& item)
    {
        if (item.getType() != ESM::Light::sRecordId)
            return;
        if (!obj.node()->getAuxiliary())
            return;
        ItemLights lights;
        obj.node()->getValue(sItemLights, lights);
        auto l = lights.find(item);
        if (l == lights.end())
            return;
        vsgUtil::removeChild(obj.nodeToAddChildrenTo(), l->second.node);

        auto& ctrls = obj.autoPlay.getOrCreateGroup().controllers;
        auto citr = ctrls.begin();
        while (citr != ctrls.end())
        {
            if (citr->second == l->second.node)
                citr = ctrls.erase(citr);
            else
                ++citr;
        }

        obj.node()->setValue(sItemLights, lights);
    }

    void addItemLightsAndListener(MWAnim::Object& obj, MWWorld::ContainerStore& store)
    {
        for (auto iter = store.cbegin(MWWorld::ContainerStore::Type_Light); iter != store.cend(); ++iter)
        {
            auto light = iter->get<ESM::Light>()->mBase;
            if (!(light->mData.mFlags & ESM::Light::Carry))
                addItemLight(obj, *iter, *light);
        }
        store.setContListener(new UpdateItemLights(obj));
    }

    void removeListener(MWWorld::ContainerStore& store)
    {
        if (auto listener = store.getContListener())
        {
            delete listener;
            store.setContListener(nullptr);
        }
    }

    void addItemLightIfRequired(MWAnim::Object& obj, const MWWorld::ConstPtr& item)
    {
        if (item.getType() != ESM::Light::sRecordId)
            return;
        auto light = item.get<ESM::Light>()->mBase;
        if (!(light->mData.mFlags & ESM::Light::Carry))
            addItemLight(obj, item, *light);
    }
}

#include "itemlight.hpp"

#include <vsg/nodes/Group.h>

#include <components/animation/update.hpp>
#include <components/mwanimation/light.hpp>
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
        UpdateItemLights(vsg::Group &n) : node(n) {}
        vsg::Group &node;
        void itemAdded(const MWWorld::ConstPtr& item, int count) override
        {
            addItemLightIfRequired(node, item);
        }
        void itemRemoved(const MWWorld::ConstPtr& item, int count) override
        {
            removeItemLight(node, item);
        }
    };

    void addItemLight(vsg::Group &node, const MWWorld::ConstPtr &item, const ESM::Light &light)
    {
        ItemLights lights;
        node.getValue(sItemLights, lights);
        if (lights.find(item) != lights.end())
            return;
        //osg::Vec4f ambient(1,1,1,1);
        ItemLight l;
        l.node = MWAnim::addLight(&node, &node, light, l.update.controllers);
        lights[item] = l;
        node.setValue(sItemLights, lights);
    }

    void removeItemLight(vsg::Group &node, const MWWorld::ConstPtr &item)
    {
        if (item.getType() != ESM::Light::sRecordId)
            return;
        ItemLights lights;
        node.getValue(sItemLights, lights);
        auto l = lights.find(item);
        if (l == lights.end())
            return;
        vsgUtil::removeChild(&node, l->second.node);
        node.setValue(sItemLights, lights);
    }

    void addItemLightsAndListener(vsg::Group &node, MWWorld::ContainerStore &store)
    {
        for (auto iter = store.cbegin(MWWorld::ContainerStore::Type_Light); iter != store.cend(); ++iter)
        {
            auto light = iter->get<ESM::Light>()->mBase;
            if (!(light->mData.mFlags & ESM::Light::Carry))
                addItemLight(node, *iter, *light);
        }
        store.setContListener(new UpdateItemLights(node));
    }

    void updateItemLights(vsg::Group &node, float dt)
    {
        ItemLights lights;
        node.getValue(sItemLights, lights);
        for (auto &[key, l] : lights)
            l.update.update(dt);
    }

    void removeListener(MWWorld::ContainerStore &store)
    {
        if (auto listener = store.getContListener())
        {
            delete listener;
            store.setContListener(nullptr);
        }
    }

    void addItemLightIfRequired(vsg::Group &node, const MWWorld::ConstPtr &item)
    {
        if (item.getType() != ESM::Light::sRecordId)
            return;
        auto light = item.get<ESM::Light>()->mBase;
        if (!(light->mData.mFlags & ESM::Light::Carry))
            addItemLight(node, item, *light);
    }
}

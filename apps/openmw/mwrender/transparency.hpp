#ifndef VSGOPENMW_MWRENDER_TRANSPARENCY_H
#define VSGOPENMW_MWRENDER_TRANSPARENCY_H

#include <vsg/state/BindDescriptorSet.h>
#include <vsg/nodes/StateGroup.h>

#include <components/mwanimation/object.hpp>
#include <components/pipeline/layout.hpp>
#include <components/pipeline/object.hpp>
#include <components/pipeline/sets.hpp>
#include <components/vsgutil/compilecontext.hpp>
#include <components/vsgutil/traversestate.hpp>

namespace MWRender
{
    class UpdateObjectData : public vsgUtil::TraverseState
    {
    public:
        float alpha = 1;
        UpdateObjectData()
        {
            // Some equipment pieces could be switched off temporarily, but we still want to update them.
            overrideMask = vsg::MASK_ALL;
        }
        using vsg::Visitor::apply;
        void apply(vsg::BindDescriptorSets& bds)
        {
        }
        void apply(vsg::BindDescriptorSet& bds)
        {
            if (bds.firstSet != Pipeline::OBJECT_SET || bds.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS)
                return;
            for (auto& descriptor : bds.descriptorSet->descriptors)
                descriptor->accept(*this);
        }
        void apply(vsg::DescriptorBuffer& db)
        {
            if (db.dstBinding != 0)
                return;
            if (db.bufferInfoList.empty() || !db.bufferInfoList[0]->data)
                return;
            auto& value = static_cast<vsg::Value<Pipeline::Data::Object>&>(*db.bufferInfoList[0]->data);
            value.value().alpha = alpha;
            value.dirty();
        }
    };

    class GetObjectData : public vsgUtil::TraverseState
    {
    public:
        vsg::ref_ptr<vsg::Value<Pipeline::Data::Object>> value;
        using vsg::Visitor::apply;
        void apply(vsg::BindDescriptorSets& bds)
        {
        }
        void apply(vsg::BindDescriptorSet& bds)
        {
            if (bds.firstSet != Pipeline::OBJECT_SET || bds.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS)
                return;
            for (auto& descriptor : bds.descriptorSet->descriptors)
                descriptor->accept(*this);
        }
        void apply(vsg::DescriptorBuffer& db)
        {
            if (db.dstBinding != 0)
                return;
            if (db.bufferInfoList.empty() || !db.bufferInfoList[0]->data)
                return;
            value = db.bufferInfoList[0]->data->cast<vsg::Value<Pipeline::Data::Object>>();
        }
    };

    inline float getTransparency(MWAnim::Object& obj)
    {
        if (auto sg = obj.getStateGroup())
        {
            GetObjectData visitor;
            for (auto& sc : sg->stateCommands)
                sc->accept(visitor);
            if (visitor.value)
                return visitor.value->value().alpha;
        }
        return 1.f;
    }

    inline void setTransparency(MWAnim::Object& obj, float alpha)
    {
        if (alpha == 1 && !obj.getStateGroup())
            return;
        auto sg = obj.getOrCreateStateGroup();

        UpdateObjectData visitor;
        visitor.alpha = alpha;

        if (alpha == 1)
        {
            for (auto& sc : sg->stateCommands)
                obj.context().compileContext->detach(sc);
            sg->stateCommands.clear();
            sg->traverse(visitor);
        }
        else
        {
            auto current = getTransparency(obj);
            if (current == alpha)
                return;

            if (sg->stateCommands.empty())
            {
                Pipeline::Object objectData;
                objectData.value().alpha = alpha;
                auto layout = Pipeline::getCompatiblePipelineLayout();
                sg->stateCommands = { vsg::BindDescriptorSet::create(
                    VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::OBJECT_SET, vsg::Descriptors{ objectData.descriptor() }) };
                obj.context().compileContext->compile(vsg::ref_ptr{sg});
                sg->traverse(visitor);
            }
            else
                sg->accept(visitor);
        }
    }
}

#endif

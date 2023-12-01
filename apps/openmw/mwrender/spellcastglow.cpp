#include "spellcastglow.hpp"

#include <vsg/state/StateSwitch.h>

#include <components/mwanimation/context.hpp>
#include <components/mwanimation/color.hpp>
#include <components/mwanimation/object.hpp>
#include <components/animation/tcontroller.hpp>
#include <components/animation/transform.hpp>
#include <components/vsgutil/compilecontext.hpp>

#include "env.hpp"

namespace MWRender
{
    namespace
    {
        struct Timeout : public Anim::TController<Timeout, vsg::StateSwitch>
        {
            Timeout(float in_timeout) : timeout(in_timeout) {}
            float timeout = 0;
            void apply(vsg::StateSwitch& sw, float timer) const
            {
                if (timer > timeout)
                {
                    for (auto& child : sw.children)
                        child.mask = vsg::MASK_OFF;
                }
            }
        };
    }

    void addSpellCastGlow(MWAnim::Object* object, const ESM::MagicEffect& effect, float duration)
    {
        auto color = MWAnim::color(effect.mData.mRed, effect.mData.mGreen, effect.mData.mBlue);
        auto sg = createEnv(color);
        auto envCommands = sg->stateCommands;
        if (!object->context().compileContext->compile(sg))
            return;

        auto& group = object->autoPlay.getOrCreateGroup();
        auto& controllers = group.controllers;
        vsg::ref_ptr<vsg::StateSwitch> stateSwitch;
        for (auto itr = controllers.begin(); itr != controllers.end(); ++itr)
        {
            if (auto found = dynamic_cast<const Timeout*>(itr->first))
            {
                stateSwitch = static_cast<Timeout::target_type*>(itr->second);
                controllers.erase(itr);
                break;
            }
        }
        if (!stateSwitch)
        {
            stateSwitch = vsg::StateSwitch::create();
            sg->stateCommands = { stateSwitch };
            sg->children = object->transform()->children;
            object->transform()->children = { sg };
        }
        else
        {
            for (auto& child : stateSwitch->children)
                object->context().compileContext->detach(child.stateCommand);
            stateSwitch->children.clear();
        }

        auto timeout = vsg::ref_ptr{ new Timeout(group.timer + duration) };
        for (auto& sc : envCommands)
        {
            stateSwitch->add(vsg::MASK_ALL, sc);
            stateSwitch->slot = sc->slot;
        }
        timeout->attachTo(*stateSwitch);
        controllers.emplace_back(timeout, stateSwitch);
    }
}

#include "play.hpp"

#include <limits>
#include <iostream>

#include <components/animation/reset.hpp>
#include <components/animation/controllermap.hpp>
#include <components/misc/stringops.hpp>

namespace
{
    float calcVelocity(const Anim::Tags &keys, const Anim::Channel<vsg::vec3> &channel, const vsg::vec3 &accum, const std::string &groupname)
    {
        const std::string start = groupname+": start";
        const std::string loopstart = groupname+": loop start";
        const std::string loopstop = groupname+": loop stop";
        const std::string stop = groupname+": stop";
        float starttime = std::numeric_limits<float>::max();
        float stoptime = 0.0f;

        // Pick the last Loop Stop key and the last Loop Start key.
        // This is required because of broken text keys in AshVampire.nif.
        // It has *two* WalkForward: Loop Stop keys at different times, the first one is used for stopping playback
        // but the animation velocity calculation uses the second one.
        // As result the animation velocity calculation is not correct, and this incorrect velocity must be replicated,
        // because otherwise the Creature's Speed (dagoth uthol) would not be sufficient to move fast enough.
        auto keyiter = keys.rbegin();
        while(keyiter != keys.rend())
        {
            if(keyiter->second == start || keyiter->second == loopstart)
            {
                starttime = keyiter->first;
                break;
            }
            ++keyiter;
        }
        keyiter = keys.rbegin();
        while(keyiter != keys.rend())
        {
            if (keyiter->second == stop)
                stoptime = keyiter->first;
            else if (keyiter->second == loopstop)
            {
                stoptime = keyiter->first;
                break;
            }
            ++keyiter;
        }

        if(stoptime > starttime)
        {
            auto startpos = channel.value(starttime) * accum;
            auto endpos = channel.value(stoptime) * accum;
            return vsg::length(startpos-endpos) / (stoptime - starttime);
        }
        return 0.0f;
    }

    const Anim::TransformController *&findCtrl(std::vector<Anim::TransformControllers> &vec, vsg::Object *needle)
    {
        for (auto &group : vec)
            for (auto &[ctrl,o] : group)
                if (o == needle)
                    return ctrl;
        throw std::runtime_error("!findCtrl");
    }
}

namespace MWAnim
{
    struct Play::AnimSource
    {
        std::vector<Anim::TransformControllers> tcontrollers;
        vsg::ref_ptr<const Anim::Tags> tags;
        vsg::ref_ptr<Anim::Reset> reset;
        vsg::ref_ptr<vsg::Object> resetCtrl;
    };

    Play::Play(size_t boneGroupCount)
    {
        mControllerGroups.resize(boneGroupCount);
    }

    Play::~Play()
    {
    }

    bool Play::isRotationControlled(Anim::Bone &bone) const
    {
        auto &group = mControllerGroups[bone.group];
        if (auto list = group.list)
        {
            auto node = bone.node();
            for (auto &[ctrl,o] : *list)
                if (o == node)
                    return ctrl->rotate.valid();
        }
        return false;
    }

    void Play::setAccumulation(const vsg::vec3& accum)
    {
        if (mResetAccum)
            mResetAccum->setAccumulate(accum);
    }

    void Play::addSingleAnimSource(vsg::ref_ptr<Anim::ControllerMap> map)
    {
        auto anim = std::make_unique<AnimSource>();
        anim->tcontrollers.resize(mControllerGroups.size());
        Anim::Transform *bip01{};
        Anim::Transform *root{};
        for (auto &[target, ctrl] : map->map)
        {
            if (auto bone = this->bones.search(target))
            {
                anim->tcontrollers[bone->group].emplace_back(ctrl, bone->node());
                if (Misc::StringUtils::ciEqual(target, "bip01"))
                    bip01 = bone->node();
                else if (Misc::StringUtils::ciEqual(target, "root bone"))
                    root = bone->node();
            }
            else
                std::cerr << "addAnimSource: can't find bone '" << target << "'" << std::endl;
        }
        if (!mAccumRoot)
            mAccumRoot = bip01;
        if (!mAccumRoot)
            mAccumRoot = root;
        if (mAccumRoot)
        {
            auto &replaceAccumCtrl = findCtrl(anim->tcontrollers, mAccumRoot);
            auto resetCtrl = vsg::ref_ptr{new Anim::TransformController(*replaceAccumCtrl)};
            resetCtrl->translate = anim->reset = new Anim::Reset(replaceAccumCtrl->translate);
            anim->resetCtrl = resetCtrl;
            replaceAccumCtrl = resetCtrl;
        }
        anim->tags = map->tags;
        mAnimSources.emplace_back(std::move(anim));
    }

    void Play::addAnimSources(const std::vector<vsg::ref_ptr<Anim::ControllerMap>> &sources)
    {
        for (auto &anim : sources)
            addSingleAnimSource(anim);
    }

    bool Play::hasAnimation(std::string_view anim) const
    {
        for(const auto &src : mAnimSources)
            if (src->tags->hasGroup(anim))
                return true;
        return false;
    }

    float Play::getStartTime(const std::string &groupname) const
    {
        for(AnimSourceList::const_reverse_iterator iter(mAnimSources.rbegin()); iter != mAnimSources.rend(); ++iter)
        {
            const auto found = (*iter)->tags->findGroup(groupname);
            if(found != (*iter)->tags->end())
                return found->first;
        }
        return -1.f;
    }

    float Play::getTextKeyTime(const std::string &textKey) const
    {
        for(AnimSourceList::const_reverse_iterator iter(mAnimSources.rbegin()); iter != mAnimSources.rend(); ++iter)
        {
            const auto keys = (*iter)->tags;
            for(auto iterKey = keys->begin(); iterKey != keys->end(); ++iterKey)
                if(iterKey->second.compare(0, textKey.size(), textKey) == 0)
                    return iterKey->first;
        }

        return -1.f;
    }

    void Play::handleTextKey(AnimState &state, const std::string &groupname, Anim::Tags::ConstIterator key, const Anim::Tags &map)
    {
        const std::string &evt = key->second;

        size_t off = groupname.size()+2;
        size_t len = evt.size() - off;

        if(evt.compare(0, groupname.size(), groupname) == 0 &&
           evt.compare(groupname.size(), 2, ": ") == 0)
        {
            if(evt.compare(off, len, "loop start") == 0)
                state.loopStartTime = key->first;
            else if(evt.compare(off, len, "loop stop") == 0)
                state.loopStopTime = key->first;
        }

        if (tagHandler)
        {
            try
            {
                tagHandler(groupname, key, map);
            }
            catch (std::exception& e)
            {
                std::cerr << "Error handling text key " << evt << ": " << e.what() << std::endl;
            }
        }
    }

    void Play::play(const std::string &groupname, const Priority& prio, int blendMask, bool autodisable, float speedmult,
                         const std::string &start, const std::string &stop, float startpoint, size_t loops, bool loopfallback)
    {
        if(groupname.empty())
        {
            resetActiveGroups();
            return;
        }

        Priority priority = prio;
        if (priority.mPriority.size() <= mControllerGroups.size())
            priority.mPriority.resize(mControllerGroups.size(), priority[0]);

        AnimStateMap::iterator stateiter = mStates.begin();
        while(stateiter != mStates.end())
        {
            if(stateiter->second.priority == priority)
                mStates.erase(stateiter++);
            else
                ++stateiter;
        }

        stateiter = mStates.find(groupname);
        if(stateiter != mStates.end())
        {
            stateiter->second.priority = priority;
            resetActiveGroups();
            return;
        }

        /* Look in reverse; last-inserted source has priority. */
        AnimState state(mControllerGroups.size());
        for(auto iter = mAnimSources.rbegin(); iter!=mAnimSources.rend(); ++iter)
        {
            const auto &textkeys = *((*iter)->tags);
            if(reset(state, textkeys, groupname, start, stop, startpoint, loopfallback))
            {
                state.source = iter->get();
                state.speedMult = speedmult;
                state.loopCount = loops;
                state.playing = (state.time < state.stopTime);
                state.priority = priority;
                state.blendMask = blendMask;
                state.autoDisable = autodisable;
                mStates.insert_or_assign(groupname, state);

                if (state.playing)
                {
                    auto textkey = textkeys.lowerBound(state.time);
                    while(textkey != textkeys.end() && textkey->first <= state.time)
                    {
                        handleTextKey(state, groupname, textkey, textkeys);
                        ++textkey;
                    }
                }

                if(state.time >= state.loopStopTime && state.loopCount > 0)
                {
                    state.loopCount--;
                    state.time = state.loopStartTime;
                    state.playing = true;
                    if(state.time >= state.loopStopTime)
                        break;
                    auto textkey = textkeys.lowerBound(state.time);
                    while(textkey != textkeys.end() && textkey->first <= state.time)
                    {
                        handleTextKey(state, groupname, textkey, textkeys);
                        ++textkey;
                    }
                }
                break;
            }
        }
        resetActiveGroups();
    }

    bool Play::reset(AnimState &state, const Anim::Tags &keys, const std::string &groupname, const std::string &start, const std::string &stop, float startpoint, bool loopfallback)
    {
        // Look for text keys in reverse. This normally wouldn't matter, but for some reason undeadwolf_2.nif has two
        // separate walkforward keys, and the last one is supposed to be used.
        auto groupend = keys.rbegin();
        for(;groupend != keys.rend();++groupend)
        {
            if(groupend->second.compare(0, groupname.size(), groupname) == 0 &&
               groupend->second.compare(groupname.size(), 2, ": ") == 0)
                break;
        }

        std::string starttag = groupname+": "+start;
        auto startkey = groupend;
        while(startkey != keys.rend() && startkey->second != starttag)
            ++startkey;
        if(startkey == keys.rend() && start == "loop start")
        {
            starttag = groupname+": start";
            startkey = groupend;
            while(startkey != keys.rend() && startkey->second != starttag)
                ++startkey;
        }
        if(startkey == keys.rend())
            return false;

        const std::string stoptag = groupname+": "+stop;
        auto stopkey = groupend;
        // We have to ignore extra garbage at the end.
        // The Scrib's idle3 animation has "Idle3: Stop." instead of "Idle3: Stop".
        // Why, just why? :(
        while(stopkey != keys.rend()
             && (stopkey->second.size() < stoptag.size() || stopkey->second.compare(0,stoptag.size(), stoptag) != 0))
            ++stopkey;
        if(stopkey == keys.rend())
            return false;

        if(startkey->first > stopkey->first)
            return false;

        state.startTime = startkey->first;
        if (loopfallback)
        {
            state.loopStartTime = startkey->first;
            state.loopStopTime = stopkey->first;
        }
        else
        {
            state.loopStartTime = startkey->first;
            state.loopStopTime = std::numeric_limits<float>::max();
        }
        state.stopTime = stopkey->first;

        state.time = state.startTime + ((state.stopTime - state.startTime) * startpoint);

        // mLoopStartTime and mLoopStopTime normally get assigned when encountering these keys while playing the animation
        // (see handleTextKey). But if startpoint is already past these keys, or start time is == stop time, we need to assign them now.
        const std::string loopstarttag = groupname+": loop start";
        const std::string loopstoptag = groupname+": loop stop";

        auto key = groupend;
        for (; key != startkey && key != keys.rend(); ++key)
        {
            if (key->first > state.time)
                continue;

            if (key->second == loopstarttag)
                state.loopStartTime = key->first;
            else if (key->second == loopstoptag)
                state.loopStopTime = key->first;
        }

        return true;
    }

    void Play::resetActiveGroups()
    {
        mResetAccum = nullptr;
        for(size_t i=0; i<mControllerGroups.size(); ++i)
        {
            mControllerGroups[i] = {};

            auto active = mStates.cend();
            for(auto state = mStates.cbegin();state != mStates.cend();++state)
            {
                if(!(state->second.blendMask&(1<<i)))
                    continue;

                if(active == mStates.end() || active->second.priority[i] < state->second.priority[i])
                    active = state;
            }
            if (active != mStates.end())
            {
                auto animsrc = active->second.source;
                mControllerGroups[i] = {&animsrc->tcontrollers[i], &active->second.time};
                if (i==0)
                {
                    mResetAccum = animsrc->reset;
                    if (mResetAccum)
                        mResetAccum->setAccumulate(mAccumulate);
                }
            }
        }
    }

    void Play::adjustSpeedMult(const std::string &groupname, float speedmult)
    {
        auto state(mStates.find(groupname));
        if(state != mStates.end())
            state->second.speedMult = speedmult;
    }

    bool Play::isPlaying(const std::string &groupname) const
    {
        auto state(mStates.find(groupname));
        if(state != mStates.end())
            return state->second.playing;
        return false;
    }

    bool Play::getInfo(const std::string &groupname, float *complete, float *speedmult) const
    {
        auto iter = mStates.find(groupname);
        if(iter == mStates.end())
        {
            if(complete) *complete = 0.0f;
            if(speedmult) *speedmult = 0.0f;
            return false;
        }

        if(complete)
        {
            if(iter->second.stopTime > iter->second.startTime)
                *complete = (iter->second.time - iter->second.startTime) / (iter->second.stopTime - iter->second.startTime);
            else
                *complete = (iter->second.playing ? 0.0f : 1.0f);
        }
        if(speedmult) *speedmult = iter->second.speedMult;
        return true;
    }

    float Play::getCurrentTime(const std::string &groupname) const
    {
        auto iter = mStates.find(groupname);
        if(iter == mStates.end())
            return -1.f;
        return iter->second.time;
    }

    size_t Play::getCurrentLoopCount(const std::string& groupname) const
    {
        auto iter = mStates.find(groupname);
        if(iter == mStates.end())
            return 0;
        return iter->second.loopCount;
    }

    void Play::disable(const std::string &groupname)
    {
        auto iter = mStates.find(groupname);
        if(iter != mStates.end())
            mStates.erase(iter);
        resetActiveGroups();
    }

    float Play::getVelocity(const std::string &groupname) const
    {
        if (!mAccumRoot)
            return 0.0f;

        auto found = mAnimVelocities.find(groupname);
        if (found != mAnimVelocities.end())
            return found->second;

        // Look in reverse; last-inserted source has priority.
        auto animsrc(mAnimSources.rbegin());
        for(;animsrc != mAnimSources.rend();++animsrc)
        {
            const auto &keys = *((*animsrc)->tags);
            if (keys.hasGroup(groupname))
                break;
        }
        if(animsrc == mAnimSources.rend())
            return 0.0f;

        float velocity = 0.0f;
        const auto &keys = *((*animsrc)->tags);
        if (auto reset = (*animsrc)->reset)
            velocity = calcVelocity(keys, *reset->input, mAccumulate, groupname);

        // If there's no velocity, keep looking
        if(!(velocity > 1.0f))
        {
            AnimSourceList::const_reverse_iterator animiter = mAnimSources.rbegin();
            while(*animiter != *animsrc)
                ++animiter;

            while(!(velocity > 1.0f) && ++animiter != mAnimSources.rend())
            {
                const auto &keys2 = *((*animiter)->tags);
                if (auto reset = (*animsrc)->reset)
                    velocity = calcVelocity(keys2, *reset->input, mAccumulate, groupname);
            }
        }
        mAnimVelocities.emplace(groupname, velocity);
        return velocity;
    }

    void Play::updatePosition(float oldtime, float newtime, vsg::vec3& position)
    {
        // Get the difference from the last update, and move the position
        auto &accumChannel = *mResetAccum->input;
        vsg::vec3 off = accumChannel.value(newtime) * mAccumulate;
        position += off - accumChannel.value(oldtime) * mAccumulate;
    }

    vsg::vec3 Play::runAnimation(float duration)
    {
        vsg::vec3 movement(0.f, 0.f, 0.f);
        AnimStateMap::iterator stateiter = mStates.begin();
        while(stateiter != mStates.end())
        {
            AnimState &state = stateiter->second;
            if (persistentPriority != -1 && !state.priority.contains(persistentPriority))
            {
                ++stateiter;
                continue;
            }

            const auto &textkeys = *state.source->tags;
            auto textkey = textkeys.upperBound(state.time);
            float timepassed = duration * state.speedMult;
            while(state.playing)
            {
                if (!state.shouldLoop())
                {
                    float targetTime = state.time + timepassed;
                    if(textkey == textkeys.end() || textkey->first > targetTime)
                    {
                        if(mResetAccum && &state.time == mControllerGroups[0].time)
                            updatePosition(state.time, targetTime, movement);
                        state.time = std::min(targetTime, state.stopTime);
                    }
                    else
                    {
                        if(mResetAccum && &state.time == mControllerGroups[0].time)
                            updatePosition(state.time, textkey->first, movement);
                        state.time = textkey->first;
                    }

                    state.playing = (state.time < state.stopTime);
                    timepassed = targetTime - state.time;

                    while(textkey != textkeys.end() && textkey->first <= state.time)
                    {
                        handleTextKey(state, stateiter->first, textkey, textkeys);
                        ++textkey;
                    }
                }
                if(state.shouldLoop())
                {
                    state.loopCount--;
                    state.time = state.loopStartTime;
                    state.playing = true;

                    textkey = textkeys.lowerBound(state.time);
                    while(textkey != textkeys.end() && textkey->first <= state.time)
                    {
                        handleTextKey(state, stateiter->first, textkey, textkeys);
                        ++textkey;
                    }
                    if(state.time >= state.loopStopTime)
                        break;
                }

                if(timepassed <= 0.0f)
                    break;
            }

            if(!state.playing && state.autoDisable)
            {
                mStates.erase(stateiter++);
                resetActiveGroups();
            }
            else
                ++stateiter;
        }
        for (auto &group : mControllerGroups)
            group.update();
        return movement;
    }

    void Play::setLoopingEnabled(const std::string &groupname, bool enabled)
    {
        auto state(mStates.find(groupname));
        if(state != mStates.end())
            state->second.loopingEnabled = enabled;
    }

    bool Play::isPriorityActive(int priority) const
    {
        for (const auto &[key, val] : mStates)
        {
            if (val.priority.contains(priority))
                return true;
        }
        return false;
    }
}

#ifndef VSGOPENMW_MWANIMATION_PLAY_H
#define VSGOPENMW_MWANIMATION_PLAY_H

#include <memory>
#include <vector>

#include <components/animation/transformupdate.hpp>
#include <components/animation/update.hpp>
#include <components/animation/tags.hpp>
#include <components/animation/bones.hpp>

namespace Anim
{
    class ControllerMap;
    class Reset;
}
namespace MWAnim
{

/*
 * Plays bone animations.
 */
class Play
{
public:
    Play(size_t boneGroupCount=1);
    ~Play();

    using TagHandler = std::function<void(std::string_view groupname, Anim::Tags::ConstIterator key, const Anim::Tags &tags)>;
    TagHandler tagHandler;

    Anim::Bones bones;

    void addSingleAnimSource(vsg::ref_ptr<Anim::ControllerMap> map);
    void addAnimSources(const std::vector<vsg::ref_ptr<Anim::ControllerMap>> &sources);

    /// Holds an animation priority value for each BoneGroup.
    struct Priority
    {
        Priority(int size, int priority)
        {
            mPriority.resize(size, priority);
        }
        Priority(int priority)
        {
            mPriority.resize(1, priority);
        }
        bool operator == (const Priority& other) const = default;
        int& operator[] (int n)
        {
            return mPriority[n];
        }
        const int& operator[] (int n) const
        {
            return mPriority[n];
        }
        bool contains(int priority) const
        {
            return std::find(mPriority.begin(), mPriority.end(), priority) != mPriority.end();
        }
        std::vector<int> mPriority;
    };

    float time() const { return *mControllerGroups[0].time; }

    bool hasAnimation(std::string_view anim) const;

    bool isRotationControlled(Anim::Bone &bone) const;

    // Specifies the axis' to accumulate on. Non-accumulated axis will just
    // move visually, but not affect the actual movement. Each x/y/z value
    // should be on the scale of 0 to 1.
    void setAccumulation(const vsg::vec3 &accum);

    /** Plays an animation.
     * \param groupname Name of the animation group to play.
     * \param priority Priority of the animation. The animation will play on
     *                 bone groups that don't have another animation set of a
     *                 higher priority.
     * \param blendMask Bone groups to play the animation on.
     * \param autodisable Automatically disable the animation when it stops
     *                    playing.
     * \param speedmult Speed multiplier for the animation.
     * \param start Key marker from which to start.
     * \param stop Key marker to stop at.
     * \param startpoint How far in between the two markers to start. 0 starts
     *                   at the start marker, 1 starts at the stop marker.
     * \param loops How many times to loop the animation. This will use the
     *              "loop start" and "loop stop" markers if they exist,
     *              otherwise it may fall back to "start" and "stop", but only if
     *              the \a loopFallback parameter is true.
     * \param loopFallback Allow looping an animation that has no loop keys, i.e. fall back to use
     *                     the "start" and "stop" keys for looping?
     */
    void play(const std::string &groupname, const Priority &priority, int blendMask, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback=false);

    /** Adjust the speed multiplier of an already playing animation.
     */
    void adjustSpeedMult (const std::string& groupname, float speedmult);

    /** Returns true if the named animation group is playing. */
    bool isPlaying(const std::string &groupname) const;

    bool isPriorityActive(int priority) const;

    /** Gets info about the given animation group.
     * \param groupname Animation group to check.
     * \param complete Stores completion amount (0 = at start key, 0.5 = half way between start and stop keys), etc.
     * \param speedmult Stores the animation speed multiplier
     * \return True if the animation is active, false otherwise.
     */
    bool getInfo(const std::string &groupname, float *complete=nullptr, float *speedmult=nullptr) const;

    /// Get the absolute position in the animation track of the first text key with the given group.
    float getStartTime(const std::string &groupname) const;

    /// Get the absolute position in the animation track of the text key
    float getTextKeyTime(const std::string &textKey) const;

    /// Get the current absolute position in the animation track for the animation that is currently playing from the given group.
    float getCurrentTime(const std::string& groupname) const;

    size_t getCurrentLoopCount(const std::string& groupname) const;

    /** Disables the specified animation group;
     * \param groupname Animation group to disable.
     */
    void disable(const std::string &groupname);

    /** Retrieves the velocity (in units per second) that the animation will move. */
    float getVelocity(const std::string &groupname) const;

    vsg::vec3 runAnimation(float duration);

    void setLoopingEnabled(const std::string &groupname, bool enabled);

    // Externally set when we have scripted animations
    int persistentPriority = -1;

private:
    Play(const Play&);
    void operator=(Play&);

    struct AnimSource;
    struct AnimState {
        AnimState(size_t numGroups) : priority(numGroups, 0) {}
        AnimSource *source = nullptr;
        float startTime = 0.f;
        float loopStartTime = 0.f;
        float loopStopTime = 0.f;
        float stopTime = 0.f;

        float time = 0.f;
        float speedMult = 1.f;

        bool playing = false;
        bool loopingEnabled = true;
        size_t loopCount = 0;

        Priority priority;
        int blendMask = 0;
        bool autoDisable = true;

        bool shouldLoop() const
        {
            return time >= loopStopTime && loopingEnabled && loopCount > 0;
        }
    };
    using AnimStateMap = std::map<std::string,AnimState>;
    AnimStateMap mStates;

    using AnimSourceList = std::vector<std::unique_ptr<AnimSource>>;
    AnimSourceList mAnimSources;

    // The node expected to accumulate movement during movement animations.
    Anim::Transform *mAccumRoot{};

    // Used to reset the position of the accumulation root every frame - the movement should be applied to the physics system
    Anim::Reset *mResetAccum{};

    std::vector<Anim::ExternalTransformUpdate> mControllerGroups;

    vsg::vec3 mAccumulate = {1.f, 1.f, 0.f};

    mutable std::map<std::string, float> mAnimVelocities;

    /* Sets the appropriate animations on the bone groups based on priority.
     */
    void resetActiveGroups();

    /* Updates the position of the accum root node for the given time, and
     * returns the wanted movement vector from the previous time. */
    void updatePosition(float oldtime, float newtime, vsg::vec3& position);

    /* Resets the animation to the time of the specified start marker, without
     * moving anything, and set the end time to the specified stop marker. If
     * the marker is not found, or if the markers are the same, it returns
     * false.
     */
    bool reset(AnimState &state, const Anim::Tags &keys, const std::string &groupname, const std::string &start, const std::string &stop, float startpoint, bool loopfallback);

    void handleTextKey(AnimState &state, const std::string &groupname, Anim::Tags::ConstIterator key, const Anim::Tags &);
};
}
#endif

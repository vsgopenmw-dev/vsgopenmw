#ifndef VSGOPENMW_VSGUTIL_TOGGLE_H
#define VSGOPENMW_VSGUTIL_TOGGLE_H

#include <vsg/nodes/Switch.h>

namespace vsgUtil
{
    /*
     * Conveniently manages a vsg::Switch child.
     *
     * Valid Usage
     * * vsgopenmw-toggle-child-index
     *   mNode's index in mSwitch's children must not change.
     */
    template <class T>
    class Toggle
    {
        vsg::ref_ptr<T> mNode;
        vsg::ref_ptr<vsg::Switch> mSwitch;
        size_t mIndex = 0;
    public:
        vsg::Mask enabledMask = vsg::MASK_ALL;
        void setup(vsg::ref_ptr<T> node, vsg::ref_ptr<vsg::Switch> sw, bool enabled)
        {
            mIndex = sw->children.size();
            sw->addChild(enabled ? enabledMask : vsg::MASK_OFF, node);
            mNode = node;
            mSwitch = sw;
        }
        void setEnabled(bool enabled)
        {
            mSwitch->children[mIndex].mask = enabled ? enabledMask : vsg::MASK_OFF;
        }
        T &operator *()
        {
            return *mNode;
        }
        const T &operator *() const
        {
            return *mNode;
        }
        T *operator ->()
        {
            return mNode.get();
        }
        const T *operator ->() const
        {
            return mNode.get();
        }
    };
}

#endif

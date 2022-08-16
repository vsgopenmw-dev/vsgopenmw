#ifndef VSGOPENMW_TERRAIN_GROUP_H
#define VSGOPENMW_TERRAIN_GROUP_H

#include <osg/Vec3f>

#include <vsg/nodes/Group.h>
#include <vsg/io/Options.h>
#include <memory>

#include <components/vsgutil/composite.hpp>

namespace Terrain
{
    class Storage;

    /**
     * @brief The basic interface for a terrain world. How the terrain chunks are paged and displayed
     *  is up to the implementation.
     */
    class Group : public vsgUtil::Composite<vsg::Group>
    {
    public:
        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        Group(Storage* storage, vsg::ref_ptr<const vsg::Options> imageOptions, vsg::ref_ptr<const vsg::Options> shaderOptions);

        float getHeightAt (const osg::Vec3f& worldPos);

        /// Load the cell into the scene graph.
        /// @note Not thread safe.
        virtual void loadCell(int x, int y) {}

        /// Remove the cell from the scene graph.
        /// @note Not thread safe.
        virtual void unloadCell(int x, int y) {}

    protected:
        Storage *mStorage;
        vsg::ref_ptr<const vsg::Options> mImageOptions;
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
    };
}

#endif

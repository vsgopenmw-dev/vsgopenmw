#ifndef VSGOPENMW_TERRAIN_GROUP_H
#define VSGOPENMW_TERRAIN_GROUP_H

#include <memory>

#include <vsg/io/Options.h>
#include <vsg/nodes/Group.h>

#include <components/vsgutil/composite.hpp>

namespace vsgUtil
{
    class Operation;
}
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
        Group(Storage* storage, vsg::ref_ptr<const vsg::Options> imageOptions,
            vsg::ref_ptr<const vsg::Options> shaderOptions);
        virtual ~Group();

        float getHeightAt(const vsg::vec3& worldPos);

        /// Load the cell into the scene graph.
        /// @note Not thread safe.
        virtual void loadCell(int x, int y) {}

        /// @note Thread safe.
        virtual vsg::ref_ptr<vsgUtil::Operation> cacheCell(int x, int y) { return {}; }

        /// Remove the cell from the scene graph.
        /// @note Not thread safe.
        virtual void unloadCell(int x, int y) {}

    protected:
        Storage* mStorage;
        vsg::ref_ptr<const vsg::Options> mImageOptions;
        vsg::ref_ptr<const vsg::Options> mShaderOptions;
    };
}

#endif

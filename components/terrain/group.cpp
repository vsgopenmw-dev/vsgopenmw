#include "group.hpp"

#include "storage.hpp"

namespace Terrain
{
    Group::Group(Storage *storage, vsg::ref_ptr<const vsg::Options> imageOptions, vsg::ref_ptr<const vsg::Options> shaderOptions)
        : mStorage(storage), mImageOptions(imageOptions), mShaderOptions(shaderOptions)
    {
    }

    float Group::getHeightAt(const osg::Vec3f &worldPos)
    {
        return mStorage->getHeightAt(worldPos);
    }
}

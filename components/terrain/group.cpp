#include "group.hpp"

#include "storage.hpp"

namespace Terrain
{
    Group::Group(
        Storage* storage, vsg::ref_ptr<const vsg::Options> imageOptions, vsg::ref_ptr<const vsg::Options> shaderOptions)
        : mStorage(storage)
        , mImageOptions(imageOptions)
        , mShaderOptions(shaderOptions)
    {
    }

    Group::~Group() {}

    float Group::getHeightAt(const vsg::vec3& worldPos)
    {
        return mStorage->getHeightAt(worldPos);
    }
}

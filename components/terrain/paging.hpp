#ifndef VSGOPENMW_TERRAIN_PAGING_H
#define VSGOPENMW_TERRAIN_PAGING_H

#include <memory>

#include <vsg/io/Options.h>
#include <vsg/state/Sampler.h>

#include <components/vsgutil/compileop.hpp>
#include <components/vsgutil/compilecontext.hpp>

#include "view.hpp"

namespace Terrain
{
    class Storage;
    class Builder;
    class ViewData;

    /**
     * @brief Terrain implementation that draws cells in batches, with geometry LOD
     * @note The const implementation must be thread safe.
     */
    class Paging
    {
    public:
        Paging(vsg::ref_ptr<vsgUtil::CompileContext> compile, vsg::ref_ptr<const vsg::Options> imageOptions, vsg::ref_ptr<const vsg::Options> shaderOptions, vsg::ref_ptr<vsg::Sampler> samplerOptions);
        ~Paging();

        float lodFactor = 1;
        int vertexLodMod = 0;

        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        void setStorage(Storage* storage);

        std::unique_ptr<View> createView() const;
        // Fill in view with just the specified cell without LOD, reusing batch data created for referenceViewDistance.
        // Return a CompileOp that can be run straight away, or added to an OperationThread if care is taken to avoid data races.
        vsg::ref_ptr<vsgUtil::CompileOp> updateViewForCell(View& v, const vsg::ivec2& cell, float referenceViewDistance) const;
        // Fill in view with LOD for specified view point and distance.
        // Return a CompileOp that can be run straight away, or added to an OperationThread if care is taken to avoid data races.
        vsg::ref_ptr<vsgUtil::CompileOp> updateView(View& view, const vsg::dvec3& viewPoint, float viewDistance, bool preload=false) const;
        void clearView(View& view) const;

        void pruneCache() const;

    protected:
        Storage* mStorage;
        std::unique_ptr<Builder> mBuilder;
        vsg::ref_ptr<vsgUtil::CompileContext> mCompile;
        const int mMaxBatchSize = 16;
        int getBatchSize(float cellViewDist)const;
    };
}

#endif

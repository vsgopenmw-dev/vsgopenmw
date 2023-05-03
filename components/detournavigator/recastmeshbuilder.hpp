#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHBUILDER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHBUILDER_H

#include "recastmesh.hpp"
#include "tilebounds.hpp"

#include <components/resource/bulletshape.hpp>

#include <osg/Vec3f>

#include <LinearMath/btTransform.h>

#include <array>
#include <memory>
#include <tuple>
#include <vector>

class btBoxShape;
class btCollisionShape;
class btCompoundShape;
class btConcaveShape;
class btHeightfieldTerrainShape;
class btTriangleCallback;

namespace DetourNavigator
{
    struct RecastMeshTriangle
    {
        AreaType mAreaType;
        std::array<osg::Vec3f, 3> mVertices;

        friend inline bool operator<(const RecastMeshTriangle& lhs, const RecastMeshTriangle& rhs)
        {
            return std::tie(lhs.mAreaType, lhs.mVertices) < std::tie(rhs.mAreaType, rhs.mVertices);
        }
    };

    class RecastMeshBuilder
    {
    public:
        explicit RecastMeshBuilder(const TileBounds& bounds) noexcept;

        void addObject(const btCollisionShape& shape, const btTransform& transform, const AreaType areaType,
            vsg::ref_ptr<const Resource::BulletShape> source, const ObjectTransform& objectTransform);

        void addObject(const btCompoundShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btConcaveShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btBoxShape& shape, const btTransform& transform, const AreaType areaType);

        void addWater(const osg::Vec2i& cellPosition, const Water& water);

        void addHeightfield(const osg::Vec2i& cellPosition, int cellSize, float height);

        void addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const float* heights, std::size_t size,
            float minHeight, float maxHeight);

        std::shared_ptr<RecastMesh> create(const Version& version) &&;

    private:
        const TileBounds mBounds;
        std::vector<RecastMeshTriangle> mTriangles;
        std::vector<CellWater> mWater;
        std::vector<Heightfield> mHeightfields;
        std::vector<FlatHeightfield> mFlatHeightfields;
        std::vector<MeshSource> mSources;

        inline void addObject(const btCollisionShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btConcaveShape& shape, const btTransform& transform, btTriangleCallback&& callback);

        void addObject(
            const btHeightfieldTerrainShape& shape, const btTransform& transform, btTriangleCallback&& callback);
    };

    Mesh makeMesh(std::vector<RecastMeshTriangle>&& triangles, const osg::Vec3f& shift = osg::Vec3f());

    Mesh makeMesh(const Heightfield& heightfield);
}

#endif

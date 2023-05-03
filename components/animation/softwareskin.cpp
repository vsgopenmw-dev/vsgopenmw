#include "softwareskin.hpp"

#include "skin.hpp"

namespace
{
    template <typename T>
    vsg::t_mat4<T> operator*(const vsg::t_mat4<T>& lhs, float rhs)
    {
        auto mat = lhs;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                mat(c, r) *= rhs;
        return mat;
    }
    template <typename T>
    vsg::t_mat4<T> operator+(const vsg::t_mat4<T>& lhs, const vsg::t_mat4<T>& rhs)
    {
        auto mat = vsg::t_mat4<T>();
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                mat(c, r) = lhs(c, r) + rhs(c, r);
        return mat;
    }
}

namespace Anim
{
    SoftwareSkin::SoftwareSkin(vsg::ref_ptr<const vsg::mat4Array> boneMatrices, vsg::ref_ptr<Skin> skin)
        : mBoneMatrices(boneMatrices)
        , mSkin(skin)
    {
    }

    void SoftwareSkin::apply(const vsg::vec3Array& in_verts)
    {
        // vsgopenmw-array-state-reuse-vertices
        proxy_vertices = vsg::vec3Array::create(in_verts.size()); // = availableVertexCache.get(minCount=in_verts.size());
        for (size_t i = 0; i < in_verts.size(); ++i)
        {
            auto& weights = mSkin->boneWeights->at(i);
            auto& indices = mSkin->boneIndices->at(i);
            auto& boneMatrices = *mBoneMatrices;
            auto skinMat = boneMatrices[static_cast<int>(indices[0])] * weights[0]
                + boneMatrices[static_cast<int>(indices[1])] * weights[1]
                + boneMatrices[static_cast<int>(indices[2])] * weights[2]
                + boneMatrices[static_cast<int>(indices[3])] * weights[3];
            proxy_vertices->at(i) = skinMat * in_verts.at(i);
        }
        vertices = proxy_vertices;
        // normals = proxy_normals;
    }
}

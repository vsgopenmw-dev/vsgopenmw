#include "softwareskin.hpp"

#include "skin.hpp"
#include "transform.hpp"

namespace
{
    template <typename T>
    vsg::t_mat4<T> operator*(const vsg::t_mat4<T>& lhs, float rhs)
    {
        auto mat = lhs;
        for (int r=0; r<4; ++r)
            for (int c=0; c<4; ++c)
                mat(c,r) = mat(c,r) * rhs;
        return mat;
    }
    template <typename T>
    vsg::t_mat4<T> operator+(const vsg::t_mat4<T>& lhs, const vsg::t_mat4<T>& rhs)
    {
        auto mat = vsg::t_mat4<T>();
        for (int r=0; r<4; ++r)
            for (int c=0; c<4; ++c)
                mat(c,r) = lhs(c,r) + rhs(c,r);
        return mat;
    }
}

namespace Anim
{
    SoftwareSkin::SoftwareSkin(const SoftwareSkin &skin, const ArrayState &arrayState)
        : vsg::ArrayState(arrayState)
        , mBoneMatrices(skin.mBoneMatrices)
        , mSkin(skin.mSkin)
    {
    }

    SoftwareSkin::SoftwareSkin(const vsg::mat4Array &boneMatrices, Skin &skin)
        : mBoneMatrices(boneMatrices)
        , mSkin(skin)
    {
    }

    void SoftwareSkin::apply(const vsg::vec3Array &in_verts)
    {
        proxy_vertices = vsg::vec3Array::create(in_verts.size());//reuse_vertices();
        for (size_t i=0; i<in_verts.size(); ++i)
        {
            auto &weights = mSkin.boneWeights->at(i);
            auto &indices = mSkin.boneIndices->at(i);
            auto skinMat =
                mBoneMatrices[static_cast<int>(indices[0])] * weights[0] +
                mBoneMatrices[static_cast<int>(indices[1])] * weights[1] +
                mBoneMatrices[static_cast<int>(indices[2])] * weights[2] +
                mBoneMatrices[static_cast<int>(indices[3])] * weights[3];
            if (!mSkin.skinPath.empty())
                skinMat = mSkin.inverseSkinPath() * skinMat;
            proxy_vertices->at(i) = skinMat * in_verts.at(i);
        }
        vertices = proxy_vertices;
        //normals = proxy_normals;
    }

    vsg::ref_ptr<vsg::ArrayState> SoftwareSkin::clone(vsg::ref_ptr<vsg::ArrayState> arrayState)
    {
        return vsg::ref_ptr<vsg::ArrayState>{new SoftwareSkin(*this, *arrayState)};
    }
}

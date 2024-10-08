#include "fogstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "savedgame.hpp"

namespace ESM
{

    void FogState::load(ESMReader& esm)
    {
        if (esm.isNextSub("BOUN"))
            esm.getHT(mBounds.mMinX, mBounds.mMinY, mBounds.mMaxX, mBounds.mMaxY);
        esm.getHNOT(mNorthMarkerAngle, "ANGL");
        while (esm.isNextSub("FTEX"))
        {
            esm.getSubHeader();
            FogTexture tex;

            esm.getT(tex.mX);
            esm.getT(tex.mY);

            const std::size_t imageSize = esm.getSubSize() - sizeof(int32_t) * 2;
            tex.mImageData.resize(imageSize);
            esm.getExact(tex.mImageData.data(), imageSize);

            mFogTextures.push_back(tex);
        }
    }

    void FogState::save(ESMWriter& esm, bool interiorCell) const
    {
        if (interiorCell)
        {
            esm.writeHNT("BOUN", mBounds);
            esm.writeHNT("ANGL", mNorthMarkerAngle);
        }
        for (std::vector<FogTexture>::const_iterator it = mFogTextures.begin(); it != mFogTextures.end(); ++it)
        {
            esm.startSubRecord("FTEX");
            esm.writeT(it->mX);
            esm.writeT(it->mY);
            esm.write(it->mImageData.data(), it->mImageData.size());
            esm.endRecord("FTEX");
        }
    }

}

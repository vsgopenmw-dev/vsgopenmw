#ifndef VSGOPENMW_MWRENDER_BIN_H
#define VSGOPENMW_MWRENDER_BIN_H

namespace MWRender
{
    //vsgopenmw-bin-policy=PREFER_ORDERED_NODES
    enum Bin
    {
        Bin_DepthSorted = 0,
        Bin_Compute = /*ExternalBin::*/1,
        //Bin_FirstPerson = 2,
        Bin_SunGlare = 3
    };
}

#endif

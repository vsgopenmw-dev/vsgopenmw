#ifndef VSGOPENMW_TERRAIN_GEOMETRY_H
#define VSGOPENMW_TERRAIN_GEOMETRY_H

#include <vsg/commands/Commands.h>

namespace Terrain
{
    class Storage;

    vsg::ref_ptr<vsg::Commands> createGeometry(Storage &storage, float chunkSize, const vsg::vec2 &chunkCenter, unsigned char lod, unsigned int lodFlags);
 }

#endif

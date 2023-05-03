#include "geometry.hpp"

#include <vsg/commands/BindVertexBuffers.h>
#include <vsg/commands/DrawIndexed.h>

#include <components/pipeline/mode.hpp>
#include <components/vsgutil/id.hpp>

#include "cachebuffer.hpp"
#include "storage.hpp"

namespace Terrain
{
    vsg::ref_ptr<vsg::Commands> createGeometry(
        Storage& storage, float chunkSize, const vsg::vec2& chunkCenter, unsigned char lod, unsigned int lodFlags)
    {
        vsg::ref_ptr<vsg::floatArray> heights;
        vsg::ref_ptr<vsg::vec3Array> normals;
        vsg::ref_ptr<vsg::ubvec4Array> colors;
        storage.fillVertexBuffers(lod, chunkSize, vsg::vec2(chunkCenter.x, chunkCenter.y), heights, normals, colors);
        vsgUtil::setID(*normals, static_cast<int>(Pipeline::Mode::NORMAL)); //supportIntersectNormal()

        static CacheBuffer cache;
        auto commands = vsg::Commands::create();

        unsigned int numVerts = (storage.cellVertices - 1) * chunkSize / (1 << lod) + 1;
        auto bindIndexBuffer = cache.getIndexBuffer(numVerts, lodFlags);

        vsg::DataList dataList{ heights, normals, colors };

        commands->children = { bindIndexBuffer, vsg::BindVertexBuffers::create(0, dataList),
            vsg::DrawIndexed::create(bindIndexBuffer->indices->data->valueCount(), 1, 0, 0, 0) };
        return commands;
    }
}

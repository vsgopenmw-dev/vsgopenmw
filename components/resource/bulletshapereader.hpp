#ifndef VSGOPENMW_RESOURCE_BULLETSHAPEREADER_H
#define VSGOPENMW_RESOURCE_BULLETSHAPEREADER_H

#include <stdexcept>

#include <vsg/io/ReaderWriter.h>

#include <components/nifbullet/bulletnifloader.hpp>

#include "bulletshape.hpp"

namespace Resource
{
    /*
     * Adapts nifbulletloader to vsg/ReaderWriter.
     */
    class BulletShapeReader : public vsg::ReaderWriter
    {
    public:
        vsg::ref_ptr<vsg::Object> read(std::istream& stream, vsg::ref_ptr<const vsg::Options> options) const
        {
            try
            {
                NifBullet::BulletNifLoader loader;
                // vsgopenmw-nif-istream
                Nif::NIFFile nifFile({});
                Nif::Reader reader(nifFile);
                reader.parse(Files::IStreamPtr(new std::istream(stream.rdbuf()))); //reader.parse(stream); //vsgopenmw-fixme
                Nif::FileView nif (nifFile);
                return loader.load(nif);
            }
            catch (std::exception& e)
            {
                std::cerr << e.what() << std::endl;
                return {};
            }
        }
    };
}

#endif

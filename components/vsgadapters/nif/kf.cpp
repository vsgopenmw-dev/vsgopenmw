#include "kf.hpp"

#include <iostream>

#include <components/animation/controllermap.hpp>
#include <components/nif/extra.hpp>

#include "anim.hpp"

namespace vsgAdapters
{
    vsg::ref_ptr<vsg::Object> kf::read(std::istream& stream, vsg::ref_ptr<const vsg::Options> options) const
    {
        if (!vsg::compatibleExtension(options.get(), ".kf"))
            return {};

        std::string filename;
        options->getValue("filename", filename);

        try
        {
            // vsgopenmw-nif-istream
            Nif::NIFFile nifFile({});
            Nif::Reader reader(nifFile);
            reader.parse(Files::IStreamPtr(new std::istream(stream.rdbuf()))); //reader.parse(stream); //vsgopenmw-fixme
            Nif::FileView nif (nifFile);

            auto controllers = vsg::ref_ptr{ new Anim::ControllerMap };
            for (size_t i = 0; i < nif.numRoots(); ++i)
            {
                const auto root = nif.getRoot(i);
                if (root && root->recType == Nif::RC_NiSequenceStreamHelper)
                {
                    auto seq = static_cast<const Nif::NiSequenceStreamHelper&>(*root);
                    Nif::ExtraPtr extra = seq.extra;
                    controllers->tags = handleTextKeys(static_cast<const Nif::NiTextKeyExtraData&>(*extra.getPtr()));

                    extra = extra->next;
                    Nif::ControllerPtr ctrl = seq.controller;
                    for (; !extra.empty() && !ctrl.empty() /* && (ctrl->active()*/;
                         (extra = extra->next), (ctrl = ctrl->next))
                    {
                        if (extra->recType != Nif::RC_NiStringExtraData
                            || ctrl->recType != Nif::RC_NiKeyframeController)
                            continue;
                        auto keyctrl
                            = handleKeyframeController(static_cast<const Nif::NiKeyframeController&>(*ctrl.getPtr()));
                        if (!keyctrl)
                            continue;
                        auto& strdata = static_cast<const Nif::NiStringExtraData&>(*extra.getPtr());
                        controllers->map./*insert_or_assign*/ emplace(strdata.string, std::move(keyctrl));
                    }
                }
            }
            return controllers;
        }
        catch (std::exception& e)
        {
            std::cerr << "!kf::read(" << filename << "): " << e.what() << std::endl;
            return {};
        }
    }

    bool kf::getFeatures(Features& features) const
    {
        features.extensionFeatureMap[".kf"] = READ_ISTREAM;
        return true;
    }
}

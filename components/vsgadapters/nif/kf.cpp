#include "kf.hpp"

#include <components/animation/controllermap.hpp>
#include <components/nif/extra.hpp>

#include "anim.hpp"

namespace vsgAdapters
{
    vsg::ref_ptr<vsg::Object> kf::read(std::istream &stream, vsg::ref_ptr<const vsg::Options> options) const
    {
        std::string filename;
        options->getValue("filename", filename);
        //vsgopenmw-nif-istream
        auto nif = Nif::NIFFile(Files::IStreamPtr(new std::istream(stream.rdbuf())), filename);
;
        auto controllers = vsg::ref_ptr{new Anim::ControllerMap};
        for (size_t i=0; i<nif.numRoots(); ++i)
        {
            const auto root = nif.getRoot(i);
            if (root && root->recType == Nif::RC_NiSequenceStreamHelper)
            {
                auto seq = static_cast<const Nif::NiSequenceStreamHelper&>(*root);
                Nif::ExtraPtr extra = seq.extra;
                controllers->tags = handleTextKeys(static_cast<const Nif::NiTextKeyExtraData&>(*extra.getPtr()));

                extra = extra->next;
                Nif::ControllerPtr ctrl = seq.controller;
                for(;!extra.empty() && !ctrl.empty()/* && (ctrl->active()*/; (extra=extra->next),(ctrl=ctrl->next))
                {
                    if(extra->recType != Nif::RC_NiStringExtraData || ctrl->recType != Nif::RC_NiKeyframeController)
                        continue;
                    auto keyctrl = handleKeyframeController(static_cast<const Nif::NiKeyframeController&>(*ctrl.getPtr()));
                    if (!keyctrl)
                        continue;
                    auto &strdata = static_cast<const Nif::NiStringExtraData&>(*extra.getPtr());
                    controllers->map./*insert_or_assign*/emplace(strdata.string, std::move(keyctrl));
                }
            }
        }
        return controllers;
    }

    bool kf::getFeatures(Features& features) const
    {
        features.extensionFeatureMap[".kf"] = READ_ISTREAM;
        return true;
    }
}

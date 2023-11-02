#include "nif.hpp"

#include <iostream>
#include <stack>

#include <vsg/nodes/Switch.h>
#include <vsg/state/ArrayState.h>
#include <vsg/state/BindDescriptorSet.h>
#include <vsg/state/DescriptorBuffer.h>
#include <vsg/state/DescriptorImage.h>
// #include <vsg/nodes/LOD.h>
#include <vsg/commands/BindIndexBuffer.h>
#include <vsg/commands/Draw.h>
#include <vsg/commands/Commands.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/DepthSorted.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/utils/SharedObjects.h>

#include <components/animation/billboard.hpp>
#include <components/animation/color.hpp>
#include <components/animation/cullparticles.hpp>
#include <components/animation/meta.hpp>
#include <components/animation/morph.hpp>
#include <components/animation/roll.hpp>
#include <components/animation/skin.hpp>
#include <components/animation/stateswitch.hpp>
#include <components/animation/switch.hpp>
#include <components/animation/tags.hpp>
#include <components/animation/texmat.hpp>
#include <components/animation/transformcontroller.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/nif/controlled.hpp>
#include <components/nif/effect.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/niffile.hpp>
#include <components/nif/node.hpp>
#include <components/nif/property.hpp>
#include <components/pipeline/builder.hpp>
#include <components/pipeline/graphics.hpp>
#include <components/pipeline/material.hpp>
#include <components/pipeline/override.hpp>
#include <components/pipeline/particle.hpp>
#include <components/pipeline/sets.hpp>
#include <components/vsgutil/cullnode.hpp>
#include <components/vsgutil/group.hpp>
#include <components/vsgutil/id.hpp>
#include <components/vsgutil/name.hpp>
#include <components/vsgutil/readimage.hpp>
#include <components/vsgutil/shareimage.hpp>

#include "anim.hpp"
#include "channel.hpp"
#include "particle.hpp"

namespace Pipeline::Descriptors
{
    #include <files/shaders/comp/particle/bindings.glsl>
}
namespace
{
    static const vsg::ref_ptr<vsg::ArrayState> sNullArrayState = vsg::NullArrayState::create();

    bool isTypeGeometry(int type)
    {
        switch (type)
        {
            case Nif::RC_NiTriShape:
            case Nif::RC_NiTriStrips:
            case Nif::RC_NiLines:
            case Nif::RC_BSLODTriShape:
                return true;
        }
        return false;
    }

    VkBlendFactor convertBlendFactor(int mode)
    {
        switch (mode)
        {
            case 0:
                return VK_BLEND_FACTOR_ONE;
            case 1:
                return VK_BLEND_FACTOR_ZERO;
            case 2:
                return VK_BLEND_FACTOR_SRC_COLOR;
            case 3:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case 4:
                return VK_BLEND_FACTOR_DST_COLOR;
            case 5:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case 6:
                return VK_BLEND_FACTOR_SRC_ALPHA;
            case 7:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case 8:
                return VK_BLEND_FACTOR_DST_ALPHA;
            case 9:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case 10:
                return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            default:
                return VK_BLEND_FACTOR_SRC_ALPHA;
        }
    }

    int convertTextureSlot(size_t nifSlot, Pipeline::Mode& mode)
    {
        switch (nifSlot)
        {
            case Nif::NiTexturingProperty::DarkTexture:
                mode = Pipeline::Mode::DARK_MAP;
                return Pipeline::Descriptors::DARK_UNIT;
            case Nif::NiTexturingProperty::BaseTexture:
                mode = Pipeline::Mode::DIFFUSE_MAP;
                return Pipeline::Descriptors::DIFFUSE_UNIT;
            case Nif::NiTexturingProperty::DetailTexture:
                mode = Pipeline::Mode::DETAIL_MAP;
                return Pipeline::Descriptors::DETAIL_UNIT;
            case Nif::NiTexturingProperty::DecalTexture:
                mode = Pipeline::Mode::DECAL_MAP;
                return Pipeline::Descriptors::DECAL_UNIT;
            case Nif::NiTexturingProperty::GlowTexture:
                mode = Pipeline::Mode::GLOW_MAP;
                return Pipeline::Descriptors::GLOW_UNIT;
            case Nif::NiTexturingProperty::BumpTexture:
                mode = Pipeline::Mode::BUMP_MAP;
                return Pipeline::Descriptors::BUMP_UNIT;
            default:
                return -1;
        }
    }

    template <class List, class Func>
    void handleList(const List& list, Func f)
    {
        for (size_t i = 0; i < list.size(); ++i)
            if (!list[i].empty())
                f(*list[i].getPtr());
    }

    bool canOptimize(const std::string& f)
    {
        std::string filename = Misc::StringUtils::lowerCase(f);
        size_t slashpos = filename.find_last_of("\\/");
        if (slashpos != std::string::npos && slashpos + 1 < filename.size())
        {
            std::string basename = filename.substr(slashpos + 1);
            // xmesh.nif can not be optimized because there are keyframes added in post
            if (!basename.empty() && basename[0] == 'x')
                return false;
            // NPC skeleton files can not be optimized because of keyframes added in post
            // (most of them are usually named like 'xbase_anim.nif' anyway, but not all of them :( )
            if (basename.compare(0, 9, "base_anim") == 0 || basename.compare(0, 4, "skin") == 0)
                return false;
            // For spell VFX, DummyXX nodes must remain intact. Not adding those to reservedNames to avoid being overly
            // cautious - instead, decide on filename
            if (basename.find("vfx_pattern") != std::string::npos)
                return false;
        }
        return true;
    }
}

namespace vsgAdapters
{
    class nifImpl
    {
        const vsg::Options& mOptions;
        const vsg::ref_ptr<const vsg::Options>& mImageOptions;
        bool mCanOptimize = true;
        bool mShowMarkers = false;
        int mComputeBin;
        int mDepthSortedBin;
        uint32_t mPhaseGroups = 0;
        std::string mFilename;
        std::unique_ptr<Nif::NIFFile> mNifFile;
        std::unique_ptr<Nif::FileView> mNif;
        std::string mSkinFilter;
        std::string mSkinFilter2;
        const Pipeline::Builder& mBuilder;
        const Pipeline::Override* mOverride;

    public:
        nifImpl(std::istream& stream, const vsg::Options& options, const vsg::ref_ptr<const vsg::Options>& imageOptions,
            bool showMarkers, int computeBin, int depthSortedBin, const Pipeline::Builder& builder)
            : mOptions(options)
            , mImageOptions(imageOptions)
            , mShowMarkers(showMarkers)
            , mComputeBin(computeBin)
            , mDepthSortedBin(depthSortedBin)
            , mBuilder(builder)
        {
            options.getValue("filename", mFilename);
            options.getValue("skinfilter", mSkinFilter);
            mOverride = Pipeline::Override::get(options);
            if (!mSkinFilter.empty())
                mSkinFilter2 = std::string("tri ") + mSkinFilter;
            mCanOptimize = canOptimize(mFilename);
            mNifFile.reset(new Nif::NIFFile({}));
            Nif::Reader reader(*mNifFile);
            // vsgopenmw-nif-istream
            reader.parse(Files::IStreamPtr(new std::istream(stream.rdbuf()))); //reader.parse(stream); //vsgopenmw-fixme
                                                        mNif.reset(new Nif::FileView(*mNifFile));
        }

        void warn(const std::string& msg)
        {
            std::cerr << msg << " (" << mFilename << ")" << std::endl;
        }

        template <class T>
        void share(vsg::ref_ptr<T>& obj)
        {
            vsgUtil::share_if(mOptions.sharedObjects, obj);
        }

        Anim::Contents mContents;

        using MaterialValue = vsg::Value<Pipeline::Data::Material>;
        struct NodeOptions : public Pipeline::Options
        {
            vsg::ref_ptr<vsg::StateSwitch> stateSwitch;
            vsg::Descriptors descriptors;
            std::vector<vsg::Descriptors> textureSets;
            vsg::ref_ptr<MaterialValue> material;
            vsg::ref_ptr<Anim::Color> materialController;
            float alphaTestCutoff = 1.f;
            bool depthSorted = false;
            uint32_t animFlags = 0;
            uint32_t phaseGroup = 0;
            bool filterMatched = false;
        };
        std::stack<NodeOptions> mNodeStack;
        struct ScopedPushPop
        {
            ScopedPushPop(std::stack<NodeOptions>& stack)
                : mStack(stack)
            {
                if (mStack.empty())
                    mStack.emplace();
                else
                    mStack.emplace(mStack.top());
            }
            ~ScopedPushPop() { mStack.pop(); }
            std::stack<NodeOptions>& mStack;
        };
        NodeOptions& getNodeOptions() { return mNodeStack.top(); }

        vsg::ref_ptr<vsg::Object> load()
        {
            vsg::Group::Children nodes;
            for (size_t i = 0; i < mNif->numRoots(); ++i)
            {
                if (const Nif::Node* nifNode = dynamic_cast<const Nif::Node*>(mNif->getRoot(i)))
                {
                    if (auto node = handleNode(*nifNode))
                        nodes.emplace_back(node);
                }
            }
            vsgUtil::removeGroup(nodes);

            vsg::ref_ptr<vsg::Node> ret;
            if (!mContents.contains(Anim::Contents::TransformControllers | Anim::Contents::DynamicBounds | Anim::Contents::Placeholders))
            {
                ret = vsgUtil::createCullNode(nodes);
                vsgUtil::addLeafCullNodes(ret, 2);
            }
            else
            {
                ret = vsgUtil::getNode(nodes);
                vsgUtil::addLeafCullNodes(ret);
            }
            if (!mContents.empty())
            {
                auto meta = vsg::ref_ptr{ new Anim::Meta(mContents) };
                meta->attachTo(*ret);
            }
            return ret;
        }

        bool canSkipGeometry(bool isMarker, const Nif::Node& nifNode) const
        {
            static const std::string markerName = "tri editormarker";
            static const std::string shadowName = "shadow";
            static const std::string shadowName2 = "tri shadow";
            isMarker = isMarker && Misc::StringUtils::ciCompareLen(nifNode.name, markerName, markerName.size()) == 0
                && !mShowMarkers;
            return isMarker || Misc::StringUtils::ciCompareLen(nifNode.name, shadowName, shadowName.size()) == 0
                || Misc::StringUtils::ciCompareLen(nifNode.name, shadowName2, shadowName2.size()) == 0;
        }

        bool canOptimizeTransform(const Nif::Node& nifNode)
        {
            if (!nifNode.trafo.isIdentity() || !this->mCanOptimize || hasTransformController(nifNode)
                || nifNode.useFlags & Nif::Node::Bone)
                return false;
            if (Misc::StringUtils::ciEqual(nifNode.name, "BoneOffset"))
            {
                addAnimContents(Anim::Contents::Placeholders);
                return false;
            }
            if (Misc::StringUtils::ciEqual(nifNode.name, "AttachLight"))
            {
                addAnimContents(Anim::Contents::Placeholders);
                return false;
            }
            return true;
        }

        bool canOptimizeBillboard(const Nif::Node& nifNode)
        {
            if (auto niNode = dynamic_cast<const Nif::NiNode*>(&nifNode))
            {
                const auto& children = niNode->children;
                for (size_t i = 0; i < children.size(); ++i)
                {
                    if (children[i].empty())
                        continue;
                    auto& child = *children[i].getPtr();
                    if (!canOptimizeTransform(child) || !canOptimizeBillboard(child))
                        return false;
                }
            }
            return true;
        }

        template <class ArrayType, class SourceType>
        vsg::ref_ptr<ArrayType> copyModeArray(const SourceType& vec, Pipeline::Mode mode)
        {
            getNodeOptions().shader.addMode(mode);
            return copyArray<ArrayType>(vec);
        }

        template <class Mode>
        void addModeDescriptor(vsg::ref_ptr<vsg::Descriptor> descriptor, Mode mode)
        {
            auto& nodeOptions = getNodeOptions();
            nodeOptions.shader.addMode(mode);
            nodeOptions.descriptors.emplace_back(descriptor);
        }

        void handleSkin(const Nif::NiSkinInstance& skin, vsg::ref_ptr<vsg::vec4Array> boneIndices,
            vsg::ref_ptr<vsg::vec4Array> weights)
        {
            const Nif::NiSkinData& data = *skin.data.getPtr();
            size_t numVertices = boneIndices->size();
            constexpr size_t maxInfluenceCount = 4;
            using IndexWeightList = std::vector<std::pair<uint32_t, float>>;
            std::vector<IndexWeightList> vertexWeights(numVertices);
            auto ctrl = Anim::Skin::create();
            auto& bones = ctrl->bones;
            bones.reserve(skin.bones.size());
            if (!data.trafo.isIdentity())
            {
                // openmw-4437-niskininstance-transformation
                Anim::Transform skinTransform;
                convertTrafo(skinTransform, data.trafo);
                ctrl->transform = skinTransform.t_transform(vsg::mat4());
            }
            for (unsigned char i = 0; i < skin.bones.size(); ++i)
            {
                const auto& boneData = data.bones[i];
                size_t boneIndex = bones.size();
                Anim::Transform t;
                convertTrafo(t, boneData.trafo);
                vsg::dsphere bounds(toVsg(boneData.boundSphereCenter), boneData.boundSphereRadius);
                bones.push_back({ t.t_transform(vsg::mat4()), bounds, skin.bones[i].getPtr()->name });
                for (const auto& weight : boneData.weights)
                {
                    if (weight.vertex < numVertices)
                        vertexWeights[weight.vertex].emplace_back(boneIndex, weight.weight);
                }
            }
            for (size_t vertex = 0; vertex < numVertices; ++vertex)
            {
                auto& vertexWeight = vertexWeights[vertex];
                std::sort(
                    vertexWeight.begin(), vertexWeight.end(),
                    [](auto& left, auto& right) -> auto{ return left.second > right.second; });
                vsg::vec4& indices = (*boneIndices)[vertex];
                vsg::vec4& weight = (*weights)[vertex];
                size_t influenceCount = std::min(maxInfluenceCount, vertexWeight.size());
                for (size_t i = 0; i < influenceCount; ++i)
                {
                    indices[i] = vertexWeight[i].first;
                    weight[i] = vertexWeight[i].second;
                }
            }
            ctrl->boneIndices = boneIndices;
            ctrl->boneWeights = weights;

            auto boneMatrices = vsg::mat4Array::create(bones.size());
            ctrl->attachTo(*boneMatrices);
            auto descriptor = vsg::DescriptorBuffer::create(
                boneMatrices, Pipeline::Descriptors::BONE_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            addModeDescriptor(descriptor, Pipeline::Mode::SKIN);
            addAnimContents(Anim::Contents::Skins | Anim::Contents::Controllers | Anim::Contents::DynamicBounds);
        }

        vsg::ref_ptr<vsg::Node> handleGeometry(const Nif::NiGeometry& niGeometry)
        {
            if (niGeometry.data.empty())
                return {};

            const Nif::NiGeometryData& data = *niGeometry.data.getPtr();
            handleGeometryControllers(niGeometry.controller, data.vertices.size());

            vsg::DataList dataList;
            dataList.reserve(!data.vertices.empty() + !data.normals.empty() + !data.colors.empty() + data.uvlist.size()
                + (!niGeometry.skin.empty()) * 2);

            static_assert(static_cast<int>(Pipeline::Mode::VERTEX) == 0);
            if (!data.vertices.empty())
                dataList.emplace_back(copyModeArray<vsg::vec3Array>(data.vertices, Pipeline::Mode::VERTEX));

            static_assert(static_cast<int>(Pipeline::Mode::NORMAL) == 1);
            if (!data.normals.empty())
            {
                dataList.emplace_back(copyModeArray<vsg::vec3Array>(data.normals, Pipeline::Mode::NORMAL));
                vsgUtil::setID(*dataList.back(), static_cast<int>(Pipeline::Mode::NORMAL));
            }

            static_assert(static_cast<int>(Pipeline::Mode::COLOR) == 2);
            if (!data.colors.empty())
                dataList.emplace_back(copyModeArray<vsg::vec4Array>(data.colors, Pipeline::Mode::COLOR));

            static_assert(static_cast<int>(Pipeline::Mode::SKIN) == 3);
            if (!niGeometry.skin.empty())
            {
                auto weights = vsg::vec4Array::create(data.vertices.size());
                auto boneIndices = vsg::vec4Array::create(data.vertices.size());
                handleSkin(*niGeometry.skin.getPtr(), boneIndices, weights);
                dataList.emplace_back(boneIndices);
                dataList.emplace_back(weights);
            }

            static_assert(static_cast<int>(Pipeline::Mode::TEXCOORD) == 4);
            for (size_t uvSet = 0; uvSet < data.uvlist.size(); ++uvSet)
                dataList.emplace_back(
                    copyModeArray<vsg::vec2Array>(data.uvlist[uvSet], Pipeline::Mode::TEXCOORD));
            auto& nodeOptions = getNodeOptions();
            nodeOptions.numUvSets = data.uvlist.size();

            auto& topology = nodeOptions.primitiveTopology;
            auto geom = vsg::VertexIndexDraw::create();
            geom->assignArrays(dataList);
            vsg::ref_ptr<vsg::ushortArray> indices;
            if ((niGeometry.recType == Nif::RC_NiTriShape || niGeometry.recType == Nif::RC_BSLODTriShape)
                && data.recType == Nif::RC_NiTriShapeData)
            {
                auto triangles = static_cast<const Nif::NiTriShapeData&>(data).triangles;
                if (triangles.empty())
                    return {};
                indices = copyArray<vsg::ushortArray>(triangles);
            }
            else if (niGeometry.recType == Nif::RC_NiTriStrips && data.recType == Nif::RC_NiTriStripsData)
            {
                auto triStrips = static_cast<const Nif::NiTriStripsData&>(data);
                std::vector<unsigned short> mergedIndices;
                for (const auto& strip : triStrips.strips)
                {
                    if (strip.size() < 3)
                        continue;
                    mergedIndices.insert(mergedIndices.end(), strip.begin(), strip.end());
                }
                indices = copyArray<vsg::ushortArray>(mergedIndices);
                topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            }
            else if (niGeometry.recType == Nif::RC_NiLines && data.recType == Nif::RC_NiLinesData)
            {
                const auto& line = static_cast<const Nif::NiLinesData&>(data).lines;
                if (line.empty())
                    return {};
                indices = copyArray<vsg::ushortArray>(line);
                topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            }
            else
                return {};

            geom->instanceCount = 1;
            geom->indexCount = indices->size();
            geom->assignIndices(indices);

            auto sg = createStateGroup(geom);
            if (nodeOptions.depthSorted)
            {
                // Alpha sorting uses the calculated bounding sphere centre.
                return vsg::DepthSorted::create(mDepthSortedBin, vsgUtil::getBounds(sg), sg);
            }
            return sg;
        }

        vsg::ref_ptr<vsg::Node> handleParticleSystemController(
            const Nif::NiParticles& particles, const Nif::NiParticleSystemController& partctrl)
        {
            if (particles.data.empty() || particles.data->recType != Nif::RC_NiParticlesData)
                return {};
            auto particledata = static_cast<const Nif::NiParticlesData&>(*particles.data.getPtr());
            size_t maxParticles = particledata.numParticles;
            if (maxParticles == 0)
                return {};

            auto sw = vsg::Switch::create();
            auto particleArray = vsg::Array<Pipeline::Data::Particle>::create(maxParticles);
            //particleArray->properties.dataVariance = DYNAMIC_DATA_COMPUTE;
            //particleArray->setObject(Anim::Controller::sAttachKey, vsg::ref_ptr<vsg::Object>());
            particleArray->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;

            handleInitialParticles(*particleArray, particledata, partctrl);

            auto particlesDescriptor = vsg::DescriptorBuffer::create(
                particleArray, Pipeline::Descriptors::PARTICLE_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            addModeDescriptor(particlesDescriptor, Pipeline::Mode::PARTICLE);

            auto maxLifetime = ParticleSystem::calculateMaxLifetime(partctrl);
            Pipeline::Data::SimulateArgs args{};
            args.emit = handleEmitterData(partctrl);
            vsg::ref_ptr<vsg::Data> colorCurve;
            auto affectors = partctrl.affectors;
            Pipeline::ParticleModeFlags modes{};
            for (; !affectors.empty(); affectors = affectors->next)
            {
                if (affectors->recType == Nif::RC_NiParticleGrowFade)
                {
                    const auto& gf = static_cast<const Nif::NiParticleGrowFade&>(*affectors.getPtr());
                    args.size = { gf.growTime, gf.fadeTime, partctrl.size, 0 };
                    modes |= Pipeline::ParticleMode_Size;
                }
                else if (affectors->recType == Nif::RC_NiGravity)
                {
                    const auto& gr = static_cast<const Nif::NiGravity&>(*affectors.getPtr());
                    args.gravity = { .positionDecay={ toVsg(gr.mPosition), -gr.mDecay }, .directionForce={ vsg::normalize(toVsg(gr.mDirection)), gr.mForce*1.6f } };
                    if (gr.mType == 0)
                        modes |= Pipeline::ParticleMode_GravityPlane;
                    else
                        modes |= Pipeline::ParticleMode_GravityPoint;
                }
                else if (affectors->recType == Nif::RC_NiParticleColorModifier)
                {
                    const auto& cl = static_cast<const Nif::NiParticleColorModifier&>(*affectors.getPtr());
                    if (!cl.data.empty())
                    {
                        colorCurve = createColorCurve(*cl.data.getPtr(), maxLifetime, 0.04);
                        modes |= Pipeline::ParticleMode_Color;
                    }
                }
                else if (affectors->recType == Nif::RC_NiParticleRotation)
                    ; // unused
                else
                    warn("Unhandled particle modifier " + affectors->recName);
            }

            auto colliders = partctrl.colliders;
            for (; !colliders.empty(); colliders = colliders->next)
            {
                if (colliders->recType == Nif::RC_NiPlanarCollider)
                {
                    const auto& cl = static_cast<const Nif::NiPlanarCollider&>(*colliders.getPtr());
                    args.collide = { .positionBounce={ toVsg(cl.mPosition), cl.mBounceFactor }, .xVectorRadius={ toVsg(cl.mXVector), cl.mExtents.x()/2 }, .yVectorRadius={ toVsg(cl.mYVector), cl.mExtents.y()/2 }, .plane={ toVsg(cl.mPlaneNormal), cl.mPlaneDistance } };
                    modes |= Pipeline::ParticleMode_CollidePlane;
                }
                else if (colliders->recType == Nif::RC_NiSphericalCollider)
                {
                    ;//const auto& cl = static_cast<const Nif::NiSphericalCollider&>(*colliders.getPtr());
                }
                else
                    warn("Unhandled particle collider " + colliders->recName);
            }

            auto frameArgsData = vsg::Value<Pipeline::Data::FrameArgs>::create();
            auto frameArgsDescriptor = vsg::DescriptorBuffer::create(
                frameArgsData, Pipeline::Descriptors::FRAME_ARGS_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            auto argsValue = vsg::Value<Pipeline::Data::SimulateArgs>::create(args);
            share(argsValue);
            auto argsDescriptor = vsg::DescriptorBuffer::create(
                argsValue, Pipeline::Descriptors::SIMULATE_ARGS_BINDING, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            share(argsDescriptor);
            vsg::Descriptors descriptors{ argsDescriptor, frameArgsDescriptor, particlesDescriptor };
            //if (colorCurve)
            {
                if (!colorCurve)
                {
                    static const auto dummyColorCurve = vsg::vec4Array::create(1, vsg::Data::Properties(VK_FORMAT_R32G32B32A32_SFLOAT));
                    colorCurve = dummyColorCurve;
                }
                auto sampler = mBuilder.createSampler();
                sampler->addressModeU = sampler->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                share(sampler);
                descriptors.emplace_back(vsg::DescriptorImage::create(sampler, colorCurve, Pipeline::Descriptors::COLOR_CURVE_BINDING));
            }

            auto& nodeOptions = getNodeOptions();
            nodeOptions.numUvSets = 1;
            nodeOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            nodeOptions.cullMode = VK_CULL_MODE_NONE;
            bool worldSpace = !(nodeOptions.animFlags & Nif::NiNode::ParticleFlag_LocalSpace);
            if (worldSpace)
                modes |= Pipeline::ParticleMode_WorldSpace;

            auto bindComputePipeline = mBuilder.particle->getOrCreate(modes);
            auto computeBds = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_COMPUTE, bindComputePipeline->pipeline->layout, Pipeline::TEXTURE_SET, descriptors);
            auto computeGroup = vsg::Commands::create(); // vsg::StateGroup::create();
            computeGroup->children = {
                bindComputePipeline,
                computeBds,
                createParticleDispatch(maxParticles)
            };

            auto draw = vsg::Draw::create(maxParticles * 6, 1, 0, 0);
            auto drawGroup = createStateGroup(draw);
            drawGroup->prototypeArrayState = sNullArrayState;
            double maxRadius = ParticleSystem::calculateRadius(partctrl);
            vsg::dsphere bound(vsg::dvec3(), maxRadius);
            // bound.dataVariance = DYNAMIC;

            sw->children = {
                { vsg::MASK_ALL, vsg::DepthSorted::create(mDepthSortedBin, bound, drawGroup) },
                { vsg::MASK_ALL, vsg::DepthSorted::create(mComputeBin, bound, computeGroup) }
            };

            auto ctrl = ParticleSystem::create(partctrl, particles.recIndex, worldSpace);
            setup(partctrl, *ctrl, *frameArgsData);

            auto cullCtrl = Anim::CullParticles::create();
            cullCtrl->active = ctrl->active;
            cullCtrl->maxLifetime = maxLifetime;
            cullCtrl->drawIndex = 0;
            cullCtrl->dispatchIndex = 1;
            setup(partctrl, *cullCtrl, *sw);

            addAnimContents(Anim::Contents::Particles | Anim::Contents::DynamicBounds);
            //return sw;
            return vsg::CullNode::create(bound, sw);
        }

        vsg::ref_ptr<vsg::Node> handleParticles(const Nif::NiParticles& particles)
        {
            const Nif::NiParticleSystemController* partctrl = nullptr;
            callActiveControllers(particles.controller, [&partctrl](auto& ctrl) {
                if (ctrl.recType == Nif::RC_NiParticleSystemController || ctrl.recType == Nif::RC_NiBSPArrayController)
                    partctrl = &static_cast<const Nif::NiParticleSystemController&>(ctrl);
            });
            if (partctrl)
                return handleParticleSystemController(particles, *partctrl);
            return {};
        }

        void handleFlipController(const Nif::NiFlipController& flipctrl)
        {
            auto& stateSwitch = getNodeOptions().stateSwitch = vsg::StateSwitch::create();
            auto ctrl = Anim::StateSwitch::create();
            ctrl->index = Anim::make_channel<FlipChannel>(flipctrl.mDelta, flipctrl.mSources.size());
            setup(flipctrl, *ctrl, *stateSwitch);
        }

        vsg::ref_ptr<vsg::Data> handleSourceTexture(const Nif::NiSourceTexture& st)
        {
            if (!st.external && !st.data.empty())
            {
                warn("vsgopenmw-testing(!st->external)");
                // handleInternalTexture(st->data.getPtr());
                return {};
            }
            else
                return vsgUtil::readImage(st.filename, mImageOptions);
        }

        vsg::ref_ptr<vsg::DescriptorImage> handleTexture(const Nif::NiSourceTexture& st, int clamp, uint32_t binding)
        {
            auto textureData = handleSourceTexture(st);
            if (!textureData)
                return {};
            auto sampler = mBuilder.createSampler();
            sampler->addressModeU
                = (clamp >> 1) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler->addressModeV
                = clamp & 0x1 ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            auto descriptor = vsgUtil::sharedDescriptorImage(mOptions.sharedObjects, sampler, textureData, binding);
            return descriptor;
        }

        void handleTextureProperty(const Nif::NiTexturingProperty& texprop)
        {
            auto& nodeOptions = getNodeOptions();
            nodeOptions.textureSets.clear();
            nodeOptions.nonStandardUvSets.clear();
            nodeOptions.stateSwitch = {};
            vsg::Descriptors textures;
            auto flipctrl = searchController<Nif::NiFlipController>(texprop.controller, Nif::RC_NiFlipController);
            size_t i = 0;
            if (flipctrl)
            {
                handleFlipController(*flipctrl);
                i = 1;
            }
            for (; i < texprop.textures.size(); ++i)
            {
                if (texprop.textures[i].inUse && !texprop.textures[i].texture.empty())
                {
                    Pipeline::Mode mode = Pipeline::Mode::DIFFUSE_MAP;
                    int binding = convertTextureSlot(i, mode);
                    if (binding == -1)
                    {
                        warn("!convertTextureSlot(NiTexturingProperty::" + std::to_string(i) + ")");
                        continue;
                    }
                    auto tex = texprop.textures[i];
                    if (tex.uvSet != 0)
                    {
                        nodeOptions.nonStandardUvSets[binding] = tex.uvSet;
                        warn("vsgopenmw-testing(tex.uvSet)");
                    }
                    if (auto descriptor = handleTexture(*tex.texture.getPtr(), tex.clamp, binding))
                    {
                        nodeOptions.shader.addMode(mode);
                        textures.emplace_back(descriptor);
                    }
                }
            }
            if (flipctrl)
            {
                handleList(flipctrl->mSources, [this, &textures, &nodeOptions, texprop](auto& n) {
                    if (auto tex = handleTexture(n, texprop.textures[0].clamp, 0))
                    {
                        nodeOptions.shader.addMode(Pipeline::Mode::DIFFUSE_MAP);
                        vsg::Descriptors textureSet{ tex };
                        std::copy(textures.begin(), textures.end(), std::back_inserter(textureSet));
                        nodeOptions.textureSets.emplace_back(textureSet);
                    }
                });
            }
            else
                nodeOptions.textureSets.emplace_back(textures);
        }

        void handleMaterialProperty(const Nif::NiMaterialProperty& matprop)
        {
            auto& nodeOptions = getNodeOptions();
            nodeOptions.material = MaterialValue::create(Pipeline::Material::createDefault());
            auto& val = nodeOptions.material->value();
            val.diffuse = vsg::vec4(toVsg(matprop.data.diffuse), matprop.data.alpha);
            val.ambient = vsg::vec4(toVsg(matprop.data.ambient), 1.f);
            val.emissive = vsg::vec4(toVsg(matprop.data.emissive), 1.f);
            // if (mNif->getVersion() > Nif::NIFFile::NIFVersion::VER_MW)
            // val.specular = vsg::vec4(toVsg(matprop.data.specular), 1.f);
            // val.shininess = matprop.data.glossiness;
            if (!matprop.controller.empty())
                handleMaterialController(matprop.controller);
            else
                nodeOptions.materialController = nullptr;
        }

        void handleMaterialController(const Nif::ControllerPtr& ptr)
        {
            auto color = Anim::Color::create();
            callActiveControllers(ptr, [this, &color](auto& ctrl) {
                if (ctrl.recType == Nif::RC_NiMaterialColorController)
                {
                    auto matctrl = static_cast<const Nif::NiMaterialColorController&>(ctrl);
                    if (matctrl.mData.empty())
                        return;
                    auto targetColor = matctrl.mTargetColor;
                    if (targetColor == 2 && mNif->getVersion() <= Nif::NIFFile::NIFVersion::VER_MW)
                        return;
                    size_t offset = 0;
                    switch (targetColor)
                    {
                        case 0:
                        default:
                            offset = offsetof(Pipeline::Data::Material, ambient);
                            break;
                        case 1:
                            offset = offsetof(Pipeline::Data::Material, diffuse);
                            break;
                        case 2:
                            offset = offsetof(Pipeline::Data::Material, specular);
                            break;
                        case 3:
                            offset = offsetof(Pipeline::Data::Material, emissive);
                            break;
                    }
                    color->colorOffset = offset;
                    color->color = handleKeyframes<vsg::vec3>(matctrl, matctrl.mData->mKeyList);
                    setHints(matctrl, color->hints);
                }
                else if (ctrl.recType == Nif::RC_NiAlphaController)
                {
                    auto alphactrl = static_cast<const Nif::NiAlphaController&>(ctrl);
                    if (alphactrl.mData.empty())
                        return;
                    color->alphaOffset = offsetof(Pipeline::Data::Material, diffuse);
                    color->alpha = handleKeyframes<float>(alphactrl, alphactrl.mData->mKeyList);
                    setHints(alphactrl, color->hints);
                }
            });
            addAnimContents();
            getNodeOptions().materialController = color;
        }

        void handleAlphaProperty(const Nif::NiAlphaProperty& alpha)
        {
            auto& options = getNodeOptions();
            options.blend = alpha.useAlphaBlending();
            if (options.blend)
            {
                options.srcBlendFactor = options.srcAlphaBlendFactor = convertBlendFactor(alpha.sourceBlendMode());
                options.dstBlendFactor = options.dstAlphaBlendFactor = convertBlendFactor(alpha.destinationBlendMode());
                if (/* d3d_8_1_compat && */ options.dstBlendFactor == VK_BLEND_FACTOR_DST_ALPHA)
                    options.dstBlendFactor = options.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                options.depthSorted = !alpha.noSorter();
            }
            if (alpha.useAlphaTesting())
            {
                options.alphaTestMode = alpha.alphaTestMode();
                options.alphaTestCutoff = alpha.data.threshold / 255.f;
            }
            else
                options.alphaTestMode = 0;
        }

        void handleStencilProperty(const Nif::NiStencilProperty& stencilprop)
        {
            auto& nodeOptions = getNodeOptions();
            nodeOptions.frontFace
                = stencilprop.data.drawMode == 2 ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
            nodeOptions.cullMode = stencilprop.data.drawMode == 3 ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
        }

        void handleWireframeProperty(const Nif::NiWireframeProperty& wireprop)
        {
            getNodeOptions().polygonMode = wireprop.isEnabled() ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;
        }

        void handleZBufferProperty(const Nif::NiZBufferProperty& zprop)
        {
            auto& nodeOptions = getNodeOptions();
            nodeOptions.depthTest = zprop.depthTest();
            nodeOptions.depthWrite = zprop.depthWrite();
            // if (!mw_compat) nodeOptions.depthFunc =
        }

        void handleVertexColorProperty(const Nif::NiVertexColorProperty& vertprop)
        {
            auto vertmode = static_cast<int>(vertprop.mVertexMode);
            if (static_cast<int>(vertprop.mLightingMode) == 0)
                vertmode = 0;
            getNodeOptions().colorMode = vertmode;
        }

        void handleSpecularProperty(const Nif::NiSpecularProperty& specprop)
        {
            // if (mNif->getVersion() > Nif::NIFFile::NIFVersion::VER_MW)
            // getNodeOptions().specular = specprop.isEnabled() && color != {0,0,0}
        }

        void handleProperty(const Nif::Property& property)
        {
            switch (property.recType)
            {
                case Nif::RC_NiStencilProperty:
                    handleStencilProperty(static_cast<const Nif::NiStencilProperty&>(property));
                    break;
                case Nif::RC_NiWireframeProperty:
                    handleWireframeProperty(static_cast<const Nif::NiWireframeProperty&>(property));
                    break;
                case Nif::RC_NiZBufferProperty:
                    handleZBufferProperty(static_cast<const Nif::NiZBufferProperty&>(property));
                    break;
                case Nif::RC_NiAlphaProperty:
                    handleAlphaProperty(static_cast<const Nif::NiAlphaProperty&>(property));
                    break;
                case Nif::RC_NiMaterialProperty:
                    handleMaterialProperty(static_cast<const Nif::NiMaterialProperty&>(property));
                    break;
                case Nif::RC_NiVertexColorProperty:
                    handleVertexColorProperty(static_cast<const Nif::NiVertexColorProperty&>(property));
                    break;
                case Nif::RC_NiSpecularProperty:
                    handleSpecularProperty(static_cast<const Nif::NiSpecularProperty&>(property));
                    break;
                case Nif::RC_NiTexturingProperty:
                    handleTextureProperty(static_cast<const Nif::NiTexturingProperty&>(property));
                    break;
                default:
                    break;
            }
        }

        vsg::ref_ptr<vsg::Data> createMaterialData()
        {
            auto& options = getNodeOptions();
            vsg::ref_ptr<MaterialValue> data;
            if (options.material && !Pipeline::Material::isDefault(*options.material))
                data = MaterialValue::create(*options.material);
            else
                data = MaterialValue::create(Pipeline::Material::createDefault());
            data->value().alphaTestCutoff = options.alphaTestCutoff;
            if (mOverride && mOverride->materialData)
                mOverride->materialData(data->value());
            // if (isDefault) return {}; // Increases shader variations.
            return data;
        }

        vsg::ref_ptr<vsg::DescriptorBuffer> getMaterialDescriptor()
        {
            if (mOverride && mOverride->material)
                return mOverride->material;
            vsg::ref_ptr<vsg::DescriptorBuffer> descriptor;
            auto matData = createMaterialData();
            if (matData)
            {
                auto& matCtrl = getNodeOptions().materialController;
                if (matCtrl)
                    matCtrl->attachTo(*matData);
                else
                    share(matData);

                descriptor = vsg::DescriptorBuffer::create(
                    matData, Pipeline::Descriptors::MATERIAL_BINDING, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                if (!matCtrl)
                    share(descriptor);
            }
            return descriptor;
        }

        vsg::ref_ptr<vsg::BindDescriptorSet> createBindDescriptorSet(vsg::PipelineLayout* layout, size_t i)
        {
            vsg::Descriptors descriptors;
            auto& nodeOptions = getNodeOptions();
            for (auto& descriptor : nodeOptions.descriptors)
                descriptors.emplace_back(descriptor);
            if (nodeOptions.textureSets.size() > i)
            {
                for (auto& texture : nodeOptions.textureSets[i])
                    descriptors.emplace_back(texture);
            }

            auto isDynamic = [](vsg::Descriptors& descriptors) -> bool {
                for (auto& d : descriptors)
                {
                    auto db = d->cast<vsg::DescriptorBuffer>();
                    if (db && !db->bufferInfoList.empty() && db->bufferInfoList[0]->data && db->bufferInfoList[0]->data->dynamic())
                        return true;
                }
                return false;
            };
            bool dynamic = isDynamic(descriptors);

            auto descriptorSet = vsg::DescriptorSet::create(layout->setLayouts[Pipeline::TEXTURE_SET], descriptors);
            if (!dynamic)
                share(descriptorSet);

            auto bds = vsg::BindDescriptorSet::create(
                VK_PIPELINE_BIND_POINT_GRAPHICS, layout, Pipeline::TEXTURE_SET, descriptorSet);
            if (!dynamic)
                share(bds);

            return bds;
        }

        vsg::ref_ptr<vsg::StateGroup> createStateGroup(vsg::ref_ptr<vsg::Node> child)
        {
            auto stateGroup = vsg::StateGroup::create();
            auto& options = getNodeOptions();
            if (mOverride && mOverride->pipelineOptions)
                mOverride->pipelineOptions(options);
            if (auto mat = getMaterialDescriptor())
                addModeDescriptor(mat, Pipeline::Mode::MATERIAL);
            auto bindGraphicsPipeline = mBuilder.graphics->getOrCreate(options);
            vsg::ref_ptr<vsg::PipelineLayout> layout = bindGraphicsPipeline->pipeline->layout;
            vsg::ref_ptr<vsg::StateCommand> descriptorState;
            if (options.stateSwitch)
            {
                auto stateSwitch = vsg::StateSwitch::create(*options.stateSwitch);
                for (size_t i = 0; i < options.textureSets.size(); ++i)
                {
                    auto bds = createBindDescriptorSet(layout, i);
                    stateSwitch->add(vsg::boolToMask(i == 0), bds);
                    stateSwitch->slot = bds->slot;
                }
                descriptorState = stateSwitch;
            }
            else
                descriptorState = createBindDescriptorSet(layout, 0);
            stateGroup->children = { child };
            stateGroup->stateCommands = { bindGraphicsPipeline, descriptorState };
            return stateGroup;
        }

        void setHints(const Nif::Controller& nictrl, Anim::Controller::Hints& hints)
        {
            hints.duration = std::max(hints.duration, nictrl.timeStop);
            auto& nodeOptions = getNodeOptions();
            hints.autoPlay = nodeOptions.animFlags & Nif::NiNode::AnimFlag_AutoPlay;
            hints.phaseGroup = nodeOptions.phaseGroup;
        }

        void addAnimContents(int c = Anim::Contents::Controllers) { mContents.add(c); }

        template <class Ctrl, class Target>
        void setup(const Nif::Controller& nictrl, Ctrl& controller, Target& target)
        {
            setHints(nictrl, controller.hints);
            controller.attachTo(target);
            addAnimContents();
        }

        void handleTransformControllers(const Nif::ControllerPtr controller, Anim::Transform& transform)
        {
            callActiveControllers(controller, [this, &transform](auto& ctrl) {
                if (ctrl.recType == Nif::RC_NiKeyframeController)
                {
                    if (auto keyctrl = handleKeyframeController(static_cast<const Nif::NiKeyframeController&>(ctrl)))
                    {
                        setup(ctrl, *keyctrl, transform);
                        addAnimContents(Anim::Contents::TransformControllers);
                    }
                }
                else if (ctrl.recType == Nif::RC_NiPathController)
                {
                    if (auto pathctrl = handlePathController(static_cast<const Nif::NiPathController&>(ctrl)))
                    {
                        setup(ctrl, *pathctrl, transform);
                        addAnimContents(Anim::Contents::TransformControllers);
                    }
                }
                else if (ctrl.recType == Nif::RC_NiRollController)
                {
                    if (auto rollctrl = handleRollController(static_cast<const Nif::NiRollController&>(ctrl)))
                    {
                        setup(ctrl, *rollctrl, transform);
                        addAnimContents(Anim::Contents::TransformControllers);
                    }
                }
            });
        }

        void handleUVController(const Nif::NiUVController& uvctrl)
        {
            if (uvctrl.data.empty())
                return;
            auto ctrl = Anim::TexMat::create();
            for (int i = 0; i < 2; ++i)
            {
                ctrl->translate[i] = handleKeyframes<float>(uvctrl, uvctrl.data->mKeyList[i], { 0.f });
                ctrl->scale[i] = handleKeyframes<float>(uvctrl, uvctrl.data->mKeyList[i + 2], { 1.f });
            }

            if (uvctrl.uvSet != 0)
                getNodeOptions().nonStandardUvSets[Pipeline::Descriptors::TEXMAT_BINDING] = uvctrl.uvSet;
            auto matrix = vsg::mat4Value::create();
            setup(uvctrl, *ctrl, *matrix);
            auto descriptor = vsg::DescriptorBuffer::create(
                matrix, Pipeline::Descriptors::TEXMAT_BINDING, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            addModeDescriptor(descriptor, Pipeline::Mode::TEXMAT);
        }

        void handleGeomMorpherController(const Nif::NiGeomMorpherController& morpher, size_t numVertices)
        {
            if (morpher.mData.empty())
                return;
            const auto& morphs = morpher.mData->mMorphs;
            if (morphs.empty())
                return;
            size_t numMorphs = morphs.size();
            auto array = vsg::vec4Array::create(numMorphs * numVertices);
            for (size_t vertex = 0; vertex < numVertices; ++vertex)
            {
                for (size_t j = 0; j < numMorphs; ++j)
                {
                    if (vertex < std::min(numVertices, morphs[j].mVertices.size()))
                        (*array)[vertex * numMorphs + j]
                            = vsg::vec4(toVsg(morphs[j].mVertices[vertex]), static_cast<float>(numMorphs));
                }
            }

            getNodeOptions().descriptors.emplace_back(vsg::DescriptorBuffer::create(
                array, Pipeline::Descriptors::MORPH_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));

            auto weights = vsg::floatArray::create(numMorphs, 0.f);
            auto ctrl = Anim::Morph::create();
            ctrl->weights.resize(numMorphs);
            for (size_t i = 0; i < numMorphs; ++i)
            {
                float defaultWeight = i == 0 ? 1.f : 0.f; // i<relativeTargets
                weights->at(i) = defaultWeight;
                ctrl->weights[i] = handleKeyframes<float>(morpher, morphs[i].mKeyFrames, defaultWeight);
            }
            auto weightsDescriptor = vsg::DescriptorBuffer::create(
                weights, Pipeline::Descriptors::MORPH_WEIGHTS_BINDING, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            setup(morpher, *ctrl, *weights);
            addModeDescriptor(weightsDescriptor, Pipeline::Mode::MORPH);
        }

        void handleGeometryControllers(const Nif::ControllerPtr controller, size_t numVertices)
        {
            callActiveControllers(controller, [this, &numVertices](auto& ctrl) {
                if (ctrl.recType == Nif::RC_NiUVController)
                    handleUVController(static_cast<const Nif::NiUVController&>(ctrl));
                else if (ctrl.recType == Nif::RC_NiGeomMorpherController)
                    handleGeomMorpherController(static_cast<const Nif::NiGeomMorpherController&>(ctrl), numVertices);
            });
        }

        void handleVisController(const Nif::NiVisController& visctrl, vsg::Switch& sw)
        {
            auto ctrl = Anim::Switch::create(Anim::make_channel<VisChannel>(visctrl.mData->mVis));
            setup(visctrl, *ctrl, sw);
        }

        bool filterMatches(const std::string& nodeName)
        {
            return Misc::StringUtils::ciCompareLen(nodeName, mSkinFilter, mSkinFilter.size()) == 0
                || Misc::StringUtils::ciCompareLen(nodeName, mSkinFilter2, mSkinFilter2.size()) == 0;
        }

        void handleEffect(const Nif::Node& nifNode)
        {
            if (nifNode.recType != Nif::RC_NiTextureEffect)
            {
                warn("Unhandled effect " + nifNode.recName);
                return;
            }

            auto& textureEffect = static_cast<const Nif::NiTextureEffect&>(nifNode);
            if (textureEffect.textureType != Nif::NiTextureEffect::Environment_Map)
            {
                warn("Unhandled NiTextureEffect type " + std::to_string(textureEffect.textureType));
                return;
            }
            if (textureEffect.texture.empty())
                return;

            addModeDescriptor(
                handleTexture(*textureEffect.texture.getPtr(), textureEffect.clamp, Pipeline::Descriptors::ENV_UNIT),
                Pipeline::Mode::ENV_MAP);

            /*
            switch (textureEffect->coordGenType)
            {
            case Nif::NiTextureEffect::World_Parallel:
                texGen->setMode(osg::TexGen::OBJECT_LINEAR);
                break;
            case Nif::NiTextureEffect::World_Perspective:
                texGen->setMode(osg::TexGen::EYE_LINEAR);
                break;
            case Nif::NiTextureEffect::Sphere_Map:
                texGen->setMode(osg::TexGen::SPHERE_MAP);
                break;
            }
            */
        }

        void handleEffects(const Nif::NodeList& effects)
        {
            handleList(effects, [this](auto& n) { handleEffect(n); });
        }

        void handleProperties(const Nif::PropertyList& props)
        {
            handleList(props, [this](auto& n) { handleProperty(n); });
        }

        vsg::ref_ptr<vsg::Node> handleNiNodeChildren(
            const Nif::NodeList& children, vsg::ref_ptr<vsg::Group> group, bool hasMarkers, bool skipMeshes)
        {
            vsg::Group::Children vsgchildren;
            for (size_t i = 0; i < children.size(); ++i)
            {
                if (children[i].empty() || Misc::StringUtils::ciEqual(children[i]->name, "Bounding Box"))
                    continue;
                if (auto child = handleNode(*children[i].getPtr(), hasMarkers, skipMeshes))
                    vsgchildren.emplace_back(child);
            }
            if (group)
                std::copy(vsgchildren.begin(), vsgchildren.end(), std::back_inserter(group->children));
            else if (!vsgchildren.empty())
                return vsgUtil::getNode(vsgchildren);
            return {};
        }


        template <class T>
        vsg::ref_ptr<T> createGroup(const Nif::Node& nifNode) const
        {
            auto node = T::create();
            vsgUtil::setName(*node, std::string(nifNode.name));
            vsgUtil::setID(*node, nifNode.recIndex);
            return node;
        }

        vsg::ref_ptr<Anim::Transform> handleTransform(const Nif::Node& nifNode)
        {
            auto trans = createGroup<Anim::Transform>(nifNode);
            convertTrafo(*trans, nifNode.trafo);
            handleTransformControllers(nifNode.controller, *trans);
            return trans;
        }

        void handleAnimFlags(const Nif::Node& nifNode)
        {
            if (nifNode.recType == Nif::RC_NiBSAnimationNode || nifNode.recType == Nif::RC_NiBSParticleNode)
            {
                auto& nodeOptions = getNodeOptions();
                nodeOptions.animFlags = nifNode.flags;
                nodeOptions.phaseGroup = (nifNode.flags & Nif::NiNode::AnimFlag_NotRandom) ? 0u : ++mPhaseGroups; // openmw-6455-controller-random-phase
            }
        }

        vsg::ref_ptr<vsg::Node> handleNode(const Nif::Node& nifNode, bool hasMarkers = false, bool skipMeshes = false)
        {
            ScopedPushPop spp(mNodeStack);

            handleAnimFlags(nifNode);

            auto& nodeOptions = getNodeOptions();
            if (mNif->getUseSkinning() && !mSkinFilter.empty() && !nodeOptions.filterMatched)
            {
                nodeOptions.filterMatched = filterMatches(nifNode.name);
                if (!nodeOptions.filterMatched)
                {
                    if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&nifNode))
                        return handleNiNodeChildren(ninode->children, {}, false, false);
                    else
                        return {};
                }
            }

            handleProperties(nifNode.props);

            vsg::ref_ptr<vsg::Group> group; // node_to_add_children_to
            vsg::ref_ptr<vsg::Node> retNode; // topmost_node
            if (!canOptimizeTransform(nifNode))
            {
                auto trans = handleTransform(nifNode);
                trans->subgraphRequiresLocalFrustum = false;//nodeOptions.depthSorted;
                retNode = group = trans;
            }
            if (nifNode.useFlags & Nif::Node::Emitter /* || animatedCollision*/)
            {
                if (!group)
                    retNode = group = createGroup<vsg::Group>(nifNode);
            }

            if (nifNode.recType == Nif::RC_NiBillboardNode)
            {
                if (!canOptimizeBillboard(nifNode))
                {
                    auto billboard = Anim::Billboard::create();
                    billboard->subgraphRequiresLocalFrustum = false;
                    if (group)
                        billboard->addChild(group);
                    else
                        group = billboard;
                    retNode = billboard;
                }
                else
                    nodeOptions.shader.addMode(Pipeline::Mode::BILLBOARD);
            }

            for (Nif::ExtraPtr e = nifNode.extra; !e.empty(); e = e->next)
            {
                if (e->recType == Nif::RC_NiTextKeyExtraData)
                {
                    if (!group)
                        group = createGroup<vsg::Group>(nifNode);
                    handleTextKeys(static_cast<const Nif::NiTextKeyExtraData&>(*e.getPtr()))->attachTo(*group);
                }
                else if (e->recType == Nif::RC_NiStringExtraData)
                {
                    const Nif::NiStringExtraData* sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());
                    if (sd->string == "MRK")
                    {
                        // Marker objects. These meshes are only visible in the editor.
                        hasMarkers = true;
                    }
                    else if (sd->string == "BONE")
                    {
                        //;
                    }
                }
            }

            if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&nifNode))
            {
                handleEffects(ninode->effects);

                if (nifNode.recType == Nif::RC_NiSwitchNode)
                {
                    auto& niSwitchNode = static_cast<const Nif::NiSwitchNode&>(nifNode);
                    auto switchNode = vsg::Switch::create();
                    switchNode->setValue("switch", std::string(nifNode.name));
                    const Nif::NodeList& children = niSwitchNode.children;
                    switchNode->children.reserve(children.size());
                    for (size_t i = 0; i < children.size(); ++i)
                    {
                        auto child = children[i].empty() ? vsg::Node::create()
                                                         : handleNode(*children[i].getPtr(), hasMarkers, skipMeshes);
                        if (!child)
                            child = vsg::Node::create();
                        switchNode->addChild(i == niSwitchNode.initialIndex ? true : false, child);
                    }
                    if (group)
                        group->addChild(switchNode);
                    else
                        retNode = switchNode;
                }
                else if (nifNode.recType == Nif::RC_RootCollisionNode)
                {
                    // Hide collision shapes, but don't skip the subgraph
                    // We still need to animate the hidden bones so the physics system can access them
                    // addOffSwitch(
                    skipMeshes = true;
                }
                /*
                else if (nifNode.recType == Nif::RC_NiLODNode)
                {
                    auto &niLodNode = static_cast<const Nif::NiLODNode&>(nifNode);
                    vsg::ref_ptr<vsg::LOD> lodNode = vsg::LOD::create();
                    const double pixel_ratio = 1.0 / 1080.0;
                    const double angle_ratio = 1.0 / osg::DegreesToRadians(30.0); // assume a 60 fovy for reference
                    const Nif::NodeList &children = niLodNode.children;
                    double minimumScreenHeightRatio = (atan2(radius, static_cast<double>(lod.getMaxRange(i))) *
                angle_ratio);

                    for(size_t i = 0;i < children.size();++i)
                    {
                        auto child = handleNode(*children[i].getPtr(), hasMarkers, skipMeshes);
                        if (!child)
                            child = vsg::Node::create();
                        vsg::LOD::Child lodChild;
                        lodChild.node = child;
                        lodNode->addChild(lodChild);
                    }
                    if (group)
                        group->addChild(lodNode);
                    else
                     retNode = lodNode;
                }*/
                else if (auto createdNode = handleNiNodeChildren(ninode->children, group, hasMarkers, skipMeshes))
                    retNode = createdNode;
            }
            else if (isTypeGeometry(nifNode.recType) && !skipMeshes && !canSkipGeometry(hasMarkers, nifNode))
            {
                auto child = handleGeometry(static_cast<const Nif::NiGeometry&>(nifNode));
                if (group && child)
                    group->addChild(child);
                else
                    retNode = child;
            }
            else if (nifNode.recType == Nif::RC_NiParticles)
            {
                auto child = handleParticles(static_cast<const Nif::NiParticles&>(nifNode));
                if (group && child)
                    group->addChild(child);
                else if (child)
                {
                    vsgUtil::setID(*child, nifNode.recIndex);
                    retNode = child;
                }
            }
            if (group && !retNode)
                retNode = group;
            if (!retNode)
                return {};

            bool hidden = nifNode.isHidden();
            auto visctrl = searchController<Nif::NiVisController>(nifNode.controller, Nif::RC_NiVisController);
            if (hidden || (visctrl && !visctrl->mData.empty()))
            {
                auto sw = vsg::Switch::create();
                sw->children = { { !hidden, retNode } };
                if (visctrl)
                    handleVisController(*visctrl, *sw);
                retNode = sw;
            }
            return retNode;
        }
    };

    vsg::ref_ptr<vsg::Object> nif::read(std::istream& stream, vsg::ref_ptr<const vsg::Options> options) const
    {
        if (!vsg::compatibleExtension(options.get(), ".nif"))
            return {};
        try
        {
            return nifImpl(stream, *options, mImageOptions, showMarkers, mComputeBin, mDepthSortedBin, mBuilder).load();
        }
        catch (std::exception& e)
        {
            std::string filename;
            options->getValue("filename", filename);
            std::cerr << "!nif::read(" << filename << "): " << e.what() << std::endl;
            return {};
        }
    }

    bool nif::getFeatures(Features& features) const
    {
        features.extensionFeatureMap[".nif"] = READ_ISTREAM;
        // features.supportedOptions = "skinfilter"
        return true;
    }
}

#include "bindings.glsl"
#include "lib/sets.glsl"
#include "data.glsl"

#ifdef DIFFUSE_MAP
layout(set=TEXTURE_SET, binding=DIFFUSE_UNIT) uniform sampler2D diffuseMap;
#endif
#ifdef DARK_MAP
layout(set=TEXTURE_SET, binding=DARK_UNIT) uniform sampler2D darkMap;
#endif
#ifdef DETAIL_MAP
layout(set=TEXTURE_SET, binding=DETAIL_UNIT) uniform sampler2D detailMap;
#endif
#ifdef DECAL_MAP
layout(set=TEXTURE_SET, binding=DECAL_UNIT) uniform sampler2D decalMap;
#endif
#ifdef GLOW_MAP
layout(set=TEXTURE_SET, binding=GLOW_UNIT) uniform sampler2D glowMap;
#endif
#ifdef BUMP_MAP
layout(set=TEXTURE_SET, binding=BUMP_UNIT) uniform sampler2D bumpMap;
#endif
#ifdef ENV_MAP
layout(set=TEXTURE_SET, binding=ENV_UNIT) uniform sampler2D envMap;
#endif

#ifdef MATERIAL
layout(std140, set=TEXTURE_SET, binding=MATERIAL_BINDING) uniform MaterialData {
    Material material;
};
#endif

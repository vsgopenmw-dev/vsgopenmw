#include "lib/particle/data.glsl"
#include "lib/particle/util.glsl"
#include "lib/material/bindings.glsl"
#include "lib/sets.glsl"

layout(set=TEXTURE_SET, binding=PARTICLE_BINDING) buffer ParticleBuffer{
    Particle particles[];
};


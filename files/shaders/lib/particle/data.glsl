struct Particle
{
    vec4 positionSize;
    vec4 color;
    vec4 velocityAge;
    float maxAge;
    float dummy[3];
};

struct EmitArgs
{
    float horizontalDir;
    float horizontalAngle;
    float verticalDir;
    float verticalAngle;
    float velocity;
    float velocityRandom;
    float lifetime;
    float lifetimeRandom;
    vec3 offsetRandom;
};

struct FrameArgs
{
    mat4 emitMatrix;
    int emitCount;
    float dt;
    float random[2];
    float signedRandom[8];
};

struct SizeArgs
{
    float growTime;
    float fadeTime;
    float defaultSize;
};

/*
struct DrawIndirect
{
    int vertexCount;
    int instanceCount;
    int firstVertex;
    int firstInstance;
};

struct DispatchIndirect
{
    int x;
    int y;
    int z;
    int padding;
};

struct ParticleIndirectBuffer
{
    DrawIndirect drawArgs;
    DispatchIndirect emitArgs;
    DispatchIndirect simulateArgs;
    //DispatchIndirect sortArgs;
};
*/

#define COLOR_CURVE_STEP 0.015

struct FrameArgs
{
    mat4 emitMatrix;
    ivec4 emitCount;
    vec4 time;
};

struct EmitArgs
{
    vec4 offsetRandom;
    vec4 direction;
    vec4 velocityLifetime;
};

struct SizeArgs
{
    float growTime;
    float fadeTime;
    float defaultSize;
    float dummy;
};

struct GravityArgs
{
    vec4 positionDecay;
    vec4 directionForce;
};

struct CollideArgs
{
    vec4 positionBounce;
    vec4 xVectorRadius;
    vec4 yVectorRadius;
    vec4 plane;
};

struct SimulateArgs
{
    EmitArgs emit;
    SizeArgs size;
    GravityArgs gravity;
    CollideArgs collide;
};

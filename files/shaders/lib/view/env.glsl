layout(set=VIEW_SET, binding=VIEW_ENV_BINDING) uniform sampler2DArray sceneEnv;

int envIndex(float time)
{
    return int(time * 16) % VIEW_ENV_COUNT;
}

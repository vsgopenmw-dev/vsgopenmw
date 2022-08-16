#include "data.glsl"
#include "bindings.glsl"

layout(std140, set=VIEW_SET, binding=VIEW_SCENE_BINDING) uniform SceneData {
    Scene scene;
};


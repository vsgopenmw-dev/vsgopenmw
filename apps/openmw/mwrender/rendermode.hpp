#ifndef OPENMW_MWRENDER_RENDERMODE_H
#define OPENMW_MWRENDER_RENDERMODE_H

namespace MWRender
{

    enum RenderMode
    {
        Render_CollisionDebug,
        Render_Wireframe,
        Render_Pathgrid,
        Render_Water,
        Render_Scene,
        Render_NavMesh,
        Render_ActorsPaths,
        Render_RecastMesh,
        Render_Sky,
        Render_Count
    };

    enum class ViewMode
    {
        Gui,
        Scene
    };

}

#endif

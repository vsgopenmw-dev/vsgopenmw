#ifndef MWLUA_CONTEXT_H
#define MWLUA_CONTEXT_H

namespace LuaUtil
{
    class LuaState;
    class UserdataSerializer;
}
namespace MWRender
{
    class RenderManager;
}

namespace MWLua
{
    class LuaEvents;
    class LuaManager;
    class ObjectLists;

    struct Context
    {
        bool mIsGlobal;
        LuaManager* mLuaManager;
        MWRender::RenderManager* mRenderManager;
        LuaUtil::LuaState* mLua;
        LuaUtil::UserdataSerializer* mSerializer;
        ObjectLists* mObjectLists;
        LuaEvents* mLuaEvents;
    };

}

#endif // MWLUA_CONTEXT_H

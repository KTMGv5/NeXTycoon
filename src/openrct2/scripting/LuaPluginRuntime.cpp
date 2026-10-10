/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef ENABLE_SCRIPTING

#include "LuaPluginRuntime.h"
#include "Plugin.h"
#include "../Context.h"
#include "../Diagnostic.h"
#include "../Game.h"
#include "../GameState.h"
#include "../Version.h"
#include "../core/Console.hpp"
#include "../windows/Intent.h"
#include "../world/Park.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace OpenRCT2::Scripting
{
    static int LuaPrint(lua_State* L)
    {
        int n = lua_gettop(L);
        std::string line;
        for (int i = 1; i <= n; i++)
        {
            if (i > 1)
                line += " ";
            if (lua_isstring(L, i))
                line += lua_tostring(L, i);
            else if (lua_isboolean(L, i))
                line += lua_toboolean(L, i) ? "true" : "false";
            else if (lua_isnil(L, i))
                line += "nil";
            else
                line += luaL_typename(L, i);
        }
        Console::WriteLine("[Lua] %s", line.c_str());
        return 0;
    }

    static int LuaParkGetCash(lua_State* L)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(getGameState().park.cash));
        return 1;
    }

    static int LuaParkSetCash(lua_State* L)
    {
        lua_Integer cash = luaL_checkinteger(L, 1);
        getGameState().park.cash = static_cast<money64>(cash);
        if (GetContext() != nullptr)
        {
            auto intent = Intent(INTENT_ACTION_UPDATE_CASH);
            ContextBroadcastIntent(&intent);
        }
        return 0;
    }

    static int LuaParkGetRating(lua_State* L)
    {
        lua_pushinteger(L, getGameState().park.rating);
        return 1;
    }

    static int LuaParkGetGuests(lua_State* L)
    {
        lua_pushinteger(L, getGameState().park.numGuestsInPark);
        return 1;
    }

    static int LuaParkGetName(lua_State* L)
    {
        lua_pushstring(L, getGameState().park.name.c_str());
        return 1;
    }

    static int LuaGameGetTicks(lua_State* L)
    {
        lua_pushinteger(L, getGameState().currentTicks);
        return 1;
    }

    static int LuaGameIsPaused(lua_State* L)
    {
        lua_pushboolean(L, gGamePaused ? 1 : 0);
        return 1;
    }

    static int LuaGameGetVersion(lua_State* L)
    {
        lua_pushstring(L, kOpenRCT2Version);
        return 1;
    }

    static int LuaRegisterPlugin(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);

        lua_getfield(L, 1, "name");
        const char* name = luaL_optstring(L, -1, "Unnamed Lua Plugin");
        lua_pop(L, 1);

        lua_getfield(L, 1, "version");
        const char* ver = luaL_optstring(L, -1, "1.0.0");
        lua_pop(L, 1);

        lua_getfield(L, 1, "author");
        const char* author = luaL_optstring(L, -1, "Unknown");
        lua_pop(L, 1);

        auto* runtime = static_cast<LuaPluginRuntime*>(lua_touserdata(L, lua_upvalueindex(1)));
        auto* owner = static_cast<Plugin*>(lua_touserdata(L, lua_upvalueindex(2)));

        lua_getfield(L, 1, "main");
        if (lua_isfunction(L, -1))
        {
            if (runtime != nullptr)
            {
                int ref = luaL_ref(L, LUA_REGISTRYINDEX);
                runtime->SetMainRef(ref);
            }
            else
            {
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }

        if (owner != nullptr)
        {
            owner->SetLuaMetadata(name, ver, author);
        }
        return 0;
    }

    LuaPluginRuntime::LuaPluginRuntime(Plugin* owner)
        : _owner(owner)
    {
        _L = luaL_newstate();
        if (_L != nullptr)
        {
            luaL_openlibs(_L);
            RegisterApi();
        }
    }

    LuaPluginRuntime::~LuaPluginRuntime()
    {
        Stop();
        if (_L != nullptr)
        {
            lua_close(_L);
            _L = nullptr;
        }
    }

    void LuaPluginRuntime::RegisterApi()
    {
        if (_L == nullptr)
            return;

        // Custom print function
        lua_pushcfunction(_L, LuaPrint);
        lua_setglobal(_L, "print");

        // Park table
        lua_newtable(_L);
        lua_pushcfunction(_L, LuaParkGetCash);
        lua_setfield(_L, -2, "getCash");
        lua_pushcfunction(_L, LuaParkSetCash);
        lua_setfield(_L, -2, "setCash");
        lua_pushcfunction(_L, LuaParkGetRating);
        lua_setfield(_L, -2, "getRating");
        lua_pushcfunction(_L, LuaParkGetGuests);
        lua_setfield(_L, -2, "getGuests");
        lua_pushcfunction(_L, LuaParkGetName);
        lua_setfield(_L, -2, "getName");
        lua_setglobal(_L, "park");

        // Game table
        lua_newtable(_L);
        lua_pushcfunction(_L, LuaGameGetTicks);
        lua_setfield(_L, -2, "getTicks");
        lua_pushcfunction(_L, LuaGameIsPaused);
        lua_setfield(_L, -2, "isPaused");
        lua_pushcfunction(_L, LuaGameGetVersion);
        lua_setfield(_L, -2, "getVersion");
        lua_setglobal(_L, "game");

        // registerPlugin closure with runtime and owner upvalues
        lua_pushlightuserdata(_L, this);
        lua_pushlightuserdata(_L, _owner);
        lua_pushcclosure(_L, LuaRegisterPlugin, 2);
        lua_setglobal(_L, "registerPlugin");
    }

    bool LuaPluginRuntime::LoadScript(std::string_view code, std::string_view filename, std::string& outError)
    {
        if (_L == nullptr)
        {
            outError = "Lua state is null";
            return false;
        }

        std::string chunkName = "@";
        chunkName += filename;

        int status = luaL_loadbuffer(_L, code.data(), code.size(), chunkName.c_str());
        if (status != LUA_OK)
        {
            outError = lua_tostring(_L, -1);
            lua_pop(_L, 1);
            return false;
        }

        // Execute script body so registerPlugin is invoked and functions defined
        status = lua_pcall(_L, 0, 0, 0);
        if (status != LUA_OK)
        {
            outError = lua_tostring(_L, -1);
            lua_pop(_L, 1);
            return false;
        }

        return true;
    }

    bool LuaPluginRuntime::Start(std::string& outError)
    {
        if (_L == nullptr)
        {
            outError = "Lua state is null";
            return false;
        }

        if (_mainFuncRef == -1 || _mainFuncRef == -2)
        {
            // No main function registered, which is fine
            return true;
        }

        lua_rawgeti(_L, LUA_REGISTRYINDEX, _mainFuncRef);
        if (!lua_isfunction(_L, -1))
        {
            lua_pop(_L, 1);
            outError = "Main callback is not a function";
            return false;
        }

        int status = lua_pcall(_L, 0, 0, 0);
        if (status != LUA_OK)
        {
            outError = lua_tostring(_L, -1);
            lua_pop(_L, 1);
            return false;
        }

        return true;
    }

    void LuaPluginRuntime::Stop()
    {
        if (_L != nullptr && _mainFuncRef != -1 && _mainFuncRef != -2)
        {
            luaL_unref(_L, LUA_REGISTRYINDEX, _mainFuncRef);
            _mainFuncRef = -1;
        }
    }
} // namespace OpenRCT2::Scripting

#endif

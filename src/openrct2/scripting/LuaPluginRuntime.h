/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#ifdef ENABLE_SCRIPTING

#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct lua_State;

namespace OpenRCT2::Scripting
{
    class Plugin;

    class LuaPluginRuntime
    {
    private:
        lua_State* _L{ nullptr };
        Plugin* _owner{ nullptr };
        int _mainFuncRef{ -1 }; // LUA_NOREF (-2 in Lua 5.4)

    public:
        explicit LuaPluginRuntime(Plugin* owner);
        ~LuaPluginRuntime();

        LuaPluginRuntime(const LuaPluginRuntime&) = delete;
        LuaPluginRuntime& operator=(const LuaPluginRuntime&) = delete;

        bool LoadScript(std::string_view code, std::string_view filename, std::string& outError);
        bool Start(std::string& outError);
        void Stop();

        void SetMainRef(int ref)
        {
            _mainFuncRef = ref;
        }

        lua_State* GetState() const
        {
            return _L;
        }

    private:
        void RegisterApi();
    };
} // namespace OpenRCT2::Scripting

#endif

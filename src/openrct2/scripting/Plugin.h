/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#ifdef ENABLE_SCRIPTING

    #include "ScriptUtil.hpp"

    #include <memory>
    #include <optional>
    #include <quickjs.h>
    #include <string>
    #include <string_view>
    #include <vector>

namespace OpenRCT2::Scripting
{
    class LuaPluginRuntime;
    class NativePluginRuntime;

    enum class PluginType
    {
        /**
         * Scripts that can run on servers or clients with no impact on the game state and will not
         * be uploaded to clients.
         */
        local,

        /**
         * Scripts that can run on servers and will be uploaded to clients with ability to
         * modify game state in certain contexts.
         */
        remote,

        /**
         * Scripts that run when the game starts and only unload explicitly rather than when the
         * park changes.
         */
        intransient,
    };

    /**
     * Supported scripting and native programming languages in NeXTycoon.
     */
    enum class PluginLanguage
    {
        javascript, // .js (QuickJS)
        typescript, // .ts
        lua,        // .lua (Lua script runtime)
        python,     // .py (Python script runtime)
        nativeLib,  // .dll / .so (Native C/C++ C-ABI shared library)
    };

    struct PluginMetadata
    {
        std::string Name;
        std::string Version;
        std::vector<std::string> Authors;
        PluginType Type{};
        PluginLanguage Language{ PluginLanguage::javascript };
        int32_t MinApiVersion{};
        std::optional<int32_t> TargetApiVersion{};
        JSCallback Main;
    };

    class Plugin
    {
    private:
        JSContext* _context = nullptr;
        std::unique_ptr<LuaPluginRuntime> _luaRuntime;
        std::unique_ptr<NativePluginRuntime> _nativeRuntime;
        std::string _path;
        PluginMetadata _metadata{};
        PluginLanguage _language = PluginLanguage::javascript;
        std::string _code;
        bool _hasLoaded{};
        bool _hasStarted{};
        bool _isStopping{};

        std::string TryGetString(JSValue value, const char* property, const std::string& message) const;
        void DetectLanguage();
        void LoadNonJsMetadata();

    public:
        std::string_view GetPath() const
        {
            return _path;
        }

        JSContext* GetContext() const
        {
            return _context;
        }

        bool HasPath() const
        {
            return !_path.empty();
        }

        const PluginMetadata& GetMetadata() const
        {
            return _metadata;
        }

        [[nodiscard]] PluginLanguage GetLanguage() const
        {
            return _language;
        }

        void SetMetadata(JSValue obj);
        void SetLuaMetadata(std::string_view name, std::string_view version, std::string_view author);
        void SetNativeMetadata(
            std::string_view name, std::string_view version, std::string_view author, int32_t type, int32_t minApiVersion);

        const std::string& GetCode() const
        {
            return _code;
        }

        bool HasStarted() const
        {
            return _hasStarted;
        }

        bool IsStopping() const
        {
            return _isStopping;
        }

        bool IsLoaded() const
        {
            return _hasLoaded;
        }

        int32_t GetTargetAPIVersion() const;

        Plugin();
        explicit Plugin(std::string_view path);
        ~Plugin();
        Plugin(const Plugin&) = delete;
        Plugin(Plugin&&) = delete;

        void SetCode(std::string_view code);
        void Load();
        void Start();
        void StopBegin();
        void StopEnd();

        void Unload();

        bool IsTransient() const;

    private:
        void LoadCodeFromFile();

        static PluginType ParsePluginType(std::string_view type);
        static void CheckForLicence(JSValue dukLicence, std::string_view pluginName);
    };
} // namespace OpenRCT2::Scripting

#endif

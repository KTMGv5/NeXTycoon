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

#include "NeXTycoonPluginApi.h"

#include <string>
#include <string_view>

namespace OpenRCT2::Scripting
{
    class Plugin;

    class NativePluginRuntime
    {
    private:
        Plugin* _owner{ nullptr };
        void* _moduleHandle{ nullptr };
        NxPluginInitFunc _initFunc{ nullptr };
        NxPluginStartFunc _startFunc{ nullptr };
        NxPluginUpdateFunc _updateFunc{ nullptr };
        NxPluginShutdownFunc _shutdownFunc{ nullptr };
        NxPluginInfo _info{};
        NxApi _api{};

    public:
        explicit NativePluginRuntime(Plugin* owner);
        ~NativePluginRuntime();

        NativePluginRuntime(const NativePluginRuntime&) = delete;
        NativePluginRuntime& operator=(const NativePluginRuntime&) = delete;

        bool Load(std::string_view path, std::string& outError);
        bool Start(std::string& outError);
        void Update();
        void Stop();

    private:
        void InitApi();
    };
} // namespace OpenRCT2::Scripting

#endif

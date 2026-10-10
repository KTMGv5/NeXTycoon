/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef ENABLE_SCRIPTING

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "NativePluginRuntime.h"
#include "Plugin.h"
#include "../Context.h"
#include "../Diagnostic.h"
#include "../Game.h"
#include "../GameState.h"
#include "../Version.h"
#include "../core/Console.hpp"
#include "../management/NewsItem.h"
#include "../ride/RideManager.hpp"
#include "../windows/Intent.h"
#include "../world/Park.h"

namespace OpenRCT2::Scripting
{
    static void NativeLogInfo(const char* msg)
    {
        if (msg != nullptr)
        {
            Console::WriteLine("[Native Plugin] %s", msg);
            LOG_INFO("[Native Plugin] %s", msg);
        }
    }

    static void NativeLogWarning(const char* msg)
    {
        if (msg != nullptr)
        {
            Console::WriteLine("[Native Plugin WARN] %s", msg);
            LOG_WARNING("[Native Plugin] %s", msg);
        }
    }

    static void NativeLogError(const char* msg)
    {
        if (msg != nullptr)
        {
            Console::WriteLine("[Native Plugin ERROR] %s", msg);
            LOG_ERROR("[Native Plugin] %s", msg);
        }
    }

    static int32_t NativeGetParkRating(void)
    {
        return getGameState().park.rating;
    }

    static int64_t NativeGetParkCash(void)
    {
        return static_cast<int64_t>(getGameState().park.cash);
    }

    static void NativeSetParkCash(int64_t cash)
    {
        getGameState().park.cash = static_cast<money64>(cash);
        if (GetContext() != nullptr)
        {
            auto intent = Intent(INTENT_ACTION_UPDATE_CASH);
            ContextBroadcastIntent(&intent);
        }
    }

    static int32_t NativeGetGuestCount(void)
    {
        return getGameState().park.numGuestsInPark;
    }

    static const char* NativeGetParkName(void)
    {
        return getGameState().park.name.c_str();
    }

    static uint32_t NativeGetCurrentTicks(void)
    {
        return getGameState().currentTicks;
    }

    static int32_t NativeIsGamePaused(void)
    {
        return gGamePaused ? 1 : 0;
    }

    static const char* NativeGetGameVersion(void)
    {
        return kOpenRCT2Version;
    }

    static void NativeRegisterHook(const char* hook_name, void (*callback)(void* data))
    {
        if (hook_name != nullptr && callback != nullptr)
        {
            LOG_INFO("Native plugin registered hook: %s", hook_name);
        }
    }

    static int64_t NativeGetParkValue(void)
    {
        return static_cast<int64_t>(getGameState().park.value);
    }

    static int64_t NativeGetCompanyValue(void)
    {
        return static_cast<int64_t>(getGameState().park.companyValue);
    }

    static int32_t NativeGetRideCount(void)
    {
        return static_cast<int32_t>(RideManager(getGameState()).size());
    }

    static void NativePostNews(int32_t type, const char* text)
    {
        if (text != nullptr && text[0] != '\0')
        {
            News::ItemType itemType = News::ItemType::blank;
            if (type == 1)
                itemType = News::ItemType::money;
            else if (type == 2)
                itemType = News::ItemType::award;
            else if (type == 3)
                itemType = News::ItemType::graph;
            News::AddItemToQueue(getGameState().park.newsItems, itemType, text, 0);
        }
    }

    static void NativeGrantParkBonus(int64_t amount, const char* reason)
    {
        getGameState().park.cash += static_cast<money64>(amount);
        if (GetContext() != nullptr)
        {
            auto intent = Intent(INTENT_ACTION_UPDATE_CASH);
            ContextBroadcastIntent(&intent);
        }
        if (reason != nullptr && reason[0] != '\0')
        {
            std::string msg = "{GREEN}WALL STREET BONUS: {WHITE}" + std::string(reason);
            News::AddItemToQueue(getGameState().park.newsItems, News::ItemType::money, msg.c_str(), 0);
        }
    }

    NativePluginRuntime::NativePluginRuntime(Plugin* owner)
        : _owner(owner)
    {
        InitApi();
    }

    NativePluginRuntime::~NativePluginRuntime()
    {
        Stop();
    }

    void NativePluginRuntime::InitApi()
    {
        _api.struct_size = sizeof(NxApi);
        _api.api_version = NEXTYCOON_API_VERSION;
        _api.log_info = NativeLogInfo;
        _api.log_warning = NativeLogWarning;
        _api.log_error = NativeLogError;
        _api.get_park_rating = NativeGetParkRating;
        _api.get_park_cash = NativeGetParkCash;
        _api.set_park_cash = NativeSetParkCash;
        _api.get_guest_count = NativeGetGuestCount;
        _api.get_park_name = NativeGetParkName;
        _api.get_current_ticks = NativeGetCurrentTicks;
        _api.is_game_paused = NativeIsGamePaused;
        _api.get_game_version = NativeGetGameVersion;
        _api.register_hook = NativeRegisterHook;
        _api.get_park_value = NativeGetParkValue;
        _api.get_company_value = NativeGetCompanyValue;
        _api.get_ride_count = NativeGetRideCount;
        _api.post_news = NativePostNews;
        _api.grant_park_bonus = NativeGrantParkBonus;
    }

    bool NativePluginRuntime::Load(std::string_view path, std::string& outError)
    {
        std::string pathStr(path);
        HMODULE hMod = LoadLibraryA(pathStr.c_str());
        if (hMod == nullptr)
        {
            DWORD err = GetLastError();
            outError = "LoadLibrary failed with error code: " + std::to_string(err);
            return false;
        }

        _moduleHandle = static_cast<void*>(hMod);

        _initFunc = reinterpret_cast<NxPluginInitFunc>(GetProcAddress(hMod, "nx_plugin_init"));
        if (_initFunc == nullptr)
        {
            _initFunc = reinterpret_cast<NxPluginInitFunc>(GetProcAddress(hMod, "nextycoon_plugin_init"));
        }

        if (_initFunc == nullptr)
        {
            FreeLibrary(hMod);
            _moduleHandle = nullptr;
            outError = "Entry point nx_plugin_init not found in library";
            return false;
        }

        std::memset(&_info, 0, sizeof(_info));
        int32_t res = _initFunc(&_api, &_info);
        if (res != 0)
        {
            FreeLibrary(hMod);
            _moduleHandle = nullptr;
            outError = "nx_plugin_init returned non-zero error: " + std::to_string(res);
            return false;
        }

        _startFunc = reinterpret_cast<NxPluginStartFunc>(GetProcAddress(hMod, "nx_plugin_start"));
        _updateFunc = reinterpret_cast<NxPluginUpdateFunc>(GetProcAddress(hMod, "nx_plugin_update"));
        _shutdownFunc = reinterpret_cast<NxPluginShutdownFunc>(GetProcAddress(hMod, "nx_plugin_shutdown"));

        if (_owner != nullptr)
        {
            const char* name = _info.name ? _info.name : "Unnamed Native Plugin";
            const char* ver = _info.version ? _info.version : "1.0.0";
            const char* author = _info.author ? _info.author : "Unknown";
            _owner->SetNativeMetadata(name, ver, author, _info.plugin_type, _info.min_api_version);
        }

        return true;
    }

    bool NativePluginRuntime::Start([[maybe_unused]] std::string& outError)
    {
        if (_startFunc != nullptr)
        {
            _startFunc();
        }
        else if (_updateFunc != nullptr)
        {
            _updateFunc();
        }
        return true;
    }

    void NativePluginRuntime::Update()
    {
        if (_updateFunc != nullptr)
        {
            _updateFunc();
        }
    }

    void NativePluginRuntime::Stop()
    {
        if (_shutdownFunc != nullptr)
        {
            _shutdownFunc();
            _shutdownFunc = nullptr;
        }

        if (_moduleHandle != nullptr)
        {
            FreeLibrary(static_cast<HMODULE>(_moduleHandle));
            _moduleHandle = nullptr;
        }
    }
} // namespace OpenRCT2::Scripting

#endif

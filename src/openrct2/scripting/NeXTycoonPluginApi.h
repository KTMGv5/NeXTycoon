/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NEXTYCOON_API_VERSION 125

enum NxPluginType
{
    NX_PLUGIN_LOCAL = 0,
    NX_PLUGIN_REMOTE = 1,
    NX_PLUGIN_INTRANSIENT = 2,
};

typedef struct NxPluginInfo
{
    const char* name;
    const char* version;
    const char* author;
    int32_t plugin_type;
    int32_t min_api_version;
    int32_t target_api_version;
} NxPluginInfo;

typedef struct NxApi
{
    uint32_t struct_size;
    uint32_t api_version;

    // Logging
    void (*log_info)(const char* msg);
    void (*log_warning)(const char* msg);
    void (*log_error)(const char* msg);

    // Park & Game queries
    int32_t (*get_park_rating)(void);
    int64_t (*get_park_cash)(void);
    void (*set_park_cash)(int64_t cash);
    int32_t (*get_guest_count)(void);
    const char* (*get_park_name)(void);
    uint32_t (*get_current_ticks)(void);
    int32_t (*is_game_paused)(void);
    const char* (*get_game_version)(void);

    // Hooks / Event subscription
    void (*register_hook)(const char* hook_name, void (*callback)(void* data));

    // NeXTycoon v1.0.1 High-Level Extensions
    int64_t (*get_park_value)(void);
    int64_t (*get_company_value)(void);
    int32_t (*get_ride_count)(void);
    void (*post_news)(int32_t type, const char* text);
    void (*grant_park_bonus)(int64_t amount, const char* reason);
} NxApi;

// Function signatures exported by NeXTycoon native plugins (.dll / .so)
#if defined(_WIN32)
    #define NX_EXPORT __declspec(dllexport)
#else
    #define NX_EXPORT __attribute__((visibility("default")))
#endif

typedef int32_t (*NxPluginInitFunc)(const NxApi* api, NxPluginInfo* out_info);
typedef void (*NxPluginStartFunc)(void);
typedef void (*NxPluginUpdateFunc)(void);
typedef void (*NxPluginShutdownFunc)(void);

#ifdef __cplusplus
}
#endif

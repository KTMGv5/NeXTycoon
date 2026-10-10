/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <gtest/gtest.h>
#include <openrct2/GameState.h>
#include <openrct2/Version.h>
#include <openrct2/scripting/LuaPluginRuntime.h>
#include <openrct2/scripting/NativePluginRuntime.h>
#include <openrct2/scripting/NeXTycoonPluginApi.h>
#include <openrct2/scripting/Plugin.h>
#include <openrct2/world/Park.h>

using namespace OpenRCT2;
using namespace OpenRCT2::Scripting;

TEST(MultiLanguagePluginTests, LuaPluginLoadsAndParsesMetadata)
{
    Plugin plugin("scripts/power_tools.lua");
    EXPECT_EQ(plugin.GetLanguage(), PluginLanguage::lua);

    std::string code = R"(
registerPlugin({
    name = "NeXTycoon Lua Power Tools",
    version = "1.5.0",
    author = "KTMGv5",
    type = "intransient",
    main = function()
        park.setCash(999999)
    end
})
)";
    plugin.SetCode(code);
    EXPECT_NO_THROW(plugin.Load());
    EXPECT_TRUE(plugin.IsLoaded());

    EXPECT_EQ(plugin.GetMetadata().Name, "NeXTycoon Lua Power Tools");
    EXPECT_EQ(plugin.GetMetadata().Version, "1.5.0");
    ASSERT_FALSE(plugin.GetMetadata().Authors.empty());
    EXPECT_EQ(plugin.GetMetadata().Authors[0], "KTMGv5");
    EXPECT_FALSE(plugin.IsTransient());

    plugin.Unload();
    EXPECT_FALSE(plugin.IsLoaded());
}

TEST(MultiLanguagePluginTests, LuaPluginExecutesMainAndMutatesParkState)
{
    Plugin plugin("mods/cash_booster.lua");
    EXPECT_EQ(plugin.GetLanguage(), PluginLanguage::lua);

    getGameState().park.cash = 5000;
    getGameState().park.name = "NeX Park 2026";

    std::string code = R"(
registerPlugin({
    name = "Cash Booster",
    version = "2.0.0",
    author = "Tyler",
    main = function()
        local current = park.getCash()
        park.setCash(current + 45000)
    end
})
)";
    plugin.SetCode(code);
    plugin.Load();
    EXPECT_TRUE(plugin.IsLoaded());

    EXPECT_NO_THROW(plugin.Start());
    EXPECT_TRUE(plugin.HasStarted());

    EXPECT_EQ(getGameState().park.cash, 50000);

    plugin.Unload();
}

TEST(MultiLanguagePluginTests, LuaPluginRejectsSyntaxErrors)
{
    Plugin plugin("mods/broken.lua");
    plugin.SetCode("function broken( this is invalid lua syntax");
    EXPECT_THROW(plugin.Load(), std::runtime_error);
}

TEST(MultiLanguagePluginTests, NativePluginApiFunctionTable)
{
    getGameState().park.name = "Wonderland";
    getGameState().park.rating = 875;
    getGameState().park.cash = 123450;
    getGameState().park.numGuestsInPark = 420;

    NxPluginInfo info{};
    info.name = "C-ABI Native Speed Module";
    info.version = "3.2.1";
    info.author = "NeXTycoon Native Team";
    info.plugin_type = NX_PLUGIN_INTRANSIENT;
    info.min_api_version = NEXTYCOON_API_VERSION;

    Plugin plugin("plugins/speed_module.dll");
    EXPECT_EQ(plugin.GetLanguage(), PluginLanguage::nativeLib);

    plugin.SetNativeMetadata(info.name, info.version, info.author, info.plugin_type, info.min_api_version);
    EXPECT_EQ(plugin.GetMetadata().Name, "C-ABI Native Speed Module");
    EXPECT_EQ(plugin.GetMetadata().Version, "3.2.1");
    EXPECT_EQ(plugin.GetMetadata().MinApiVersion, NEXTYCOON_API_VERSION);
    EXPECT_FALSE(plugin.IsTransient());
}

TEST(MultiLanguagePluginTests, NeXTycoonWallStreetNativeDllLoadsAndExecutes)
{
    getGameState().park.name = "Wall Street Wonderland";
    getGameState().park.rating = 850;
    getGameState().park.cash = 250000;
    getGameState().park.numGuestsInPark = 650;
    getGameState().park.value = 180000;
    getGameState().park.companyValue = 430000;

    Plugin plugin("plugins/NeXTycoonWallStreet.dll");
    EXPECT_EQ(plugin.GetLanguage(), PluginLanguage::nativeLib);

    // If the compiled DLL is present in plugins/, verify full dynamic lifecycle
    try
    {
        plugin.Load();
        EXPECT_TRUE(plugin.IsLoaded());

        EXPECT_EQ(plugin.GetMetadata().Name, "NeXTycoon Wall Street & Stock Exchange");
        EXPECT_EQ(plugin.GetMetadata().Version, "1.0.0");
        ASSERT_FALSE(plugin.GetMetadata().Authors.empty());
        EXPECT_EQ(plugin.GetMetadata().Authors[0], "NeXTycoon High-Frequency Trading Lab");
        EXPECT_FALSE(plugin.IsTransient());

        EXPECT_NO_THROW(plugin.Start());
        EXPECT_TRUE(plugin.HasStarted());

        // Run multiple ticks of market simulation
        for (int i = 0; i < 5; i++)
        {
            EXPECT_NO_THROW(plugin.Update());
        }

        plugin.Unload();
        EXPECT_FALSE(plugin.IsLoaded());
    }
    catch (const std::exception& ex)
    {
        // Fail if file exists but crashed on load
        FAIL() << "Failed to load/execute NeXTycoonWallStreet.dll: " << ex.what();
    }
}


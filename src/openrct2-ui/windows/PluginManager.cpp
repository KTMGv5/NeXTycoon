/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers / NeXTycoon
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../UiStringIds.h"

#include <algorithm>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.Screen.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/drawing/Text.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/localisation/Language.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/platform/Platform.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>

#ifdef ENABLE_SCRIPTING
    #include <openrct2/scripting/Plugin.h>
    #include <openrct2/scripting/ScriptEngine.h>
#endif

#include <string>
#include <string_view>
#include <vector>

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
using namespace OpenRCT2::Ui;

namespace OpenRCT2::Ui::Windows
{
    static constexpr StringId kWindowTitle = STR_PLUGIN_MANAGER_TITLE;
    static constexpr ScreenSize kWindowSize = { 530, 340 };

    enum WindowPluginPage : uint8_t
    {
        WINDOW_PLUGIN_PAGE_PLUGINS,
        WINDOW_PLUGIN_PAGE_MODULES,
        WINDOW_PLUGIN_PAGE_COUNT,
    };

    enum WindowPluginManagerWidgetIdx : WidgetIndex
    {
        WIDX_BACKGROUND,
        WIDX_TITLE,
        WIDX_CLOSE,
        WIDX_PAGE_BACKGROUND,
        WIDX_TAB_PLUGINS,
        WIDX_TAB_MODULES,
        WIDX_GROUP_CONTENT,
        WIDX_RELOAD_PLUGINS,
        WIDX_OPEN_FOLDER,
    };

    // clang-format off
    static constexpr auto kCommonPluginWidgets = makeWidgets(
        makeWindowShim(kWindowTitle, kWindowSize),
        makeWidget({   0, 43 }, { kWindowSize.width, 297 }, WidgetType::resize, WindowColour::secondary),
        makeTab   ({   3, 17 }, STR_PLUGIN_TAB_PLUGINS),
        makeTab   ({  34, 17 }, STR_PLUGIN_TAB_MODULES)
    );

    static constexpr auto window_plugin_plugins_widgets = makeWidgets(
        kCommonPluginWidgets,
        makeWidget({   6,  48 }, { 518, 240 }, WidgetType::groupbox, WindowColour::secondary, STR_PLUGIN_TAB_PLUGINS),
        makeWidget({  14, 296 }, { 246,  24 }, WidgetType::button,   WindowColour::secondary, STR_PLUGIN_RELOAD_BTN,      STR_PLUGIN_RELOAD_TIP),
        makeWidget({ 268, 296 }, { 246,  24 }, WidgetType::button,   WindowColour::secondary, STR_PLUGIN_OPEN_FOLDER_BTN, STR_PLUGIN_OPEN_FOLDER_TIP)
    );

    static constexpr auto window_plugin_modules_widgets = makeWidgets(
        kCommonPluginWidgets,
        makeWidget({   6,  48 }, { 518, 240 }, WidgetType::groupbox, WindowColour::secondary, STR_PLUGIN_TAB_MODULES),
        makeWidget({  14, 296 }, { 500,  24 }, WidgetType::button,   WindowColour::secondary, STR_PLUGIN_OPEN_FOLDER_BTN, STR_PLUGIN_OPEN_FOLDER_TIP)
    );

    static constexpr std::span<const Widget> window_plugin_page_widgets[] = {
        window_plugin_plugins_widgets,
        window_plugin_modules_widgets,
    };
    // clang-format on

    class PluginManagerWindow final : public Window
    {
    private:
        std::string _statusMessage;

    public:
        void onOpen() override
        {
            WindowSetResize(*this, kWindowSize, kWindowSize);
            setPage(WINDOW_PLUGIN_PAGE_PLUGINS);
        }

        void setPage(uint8_t p)
        {
            if (p >= WINDOW_PLUGIN_PAGE_COUNT)
            {
                return;
            }

            page = p;
            setWidgets(window_plugin_page_widgets[page]);

            setWidgetPressed(WIDX_TAB_PLUGINS, page == WINDOW_PLUGIN_PAGE_PLUGINS);
            setWidgetPressed(WIDX_TAB_MODULES, page == WINDOW_PLUGIN_PAGE_MODULES);

            invalidate();
        }

        void onPrepareDraw() override
        {
            setWidgetPressed(WIDX_TAB_PLUGINS, page == WINDOW_PLUGIN_PAGE_PLUGINS);
            setWidgetPressed(WIDX_TAB_MODULES, page == WINDOW_PLUGIN_PAGE_MODULES);
        }

        void onMouseUp(WidgetIndex widgetIndex) override
        {
            switch (widgetIndex)
            {
                case WIDX_CLOSE:
                    close();
                    break;
                case WIDX_TAB_PLUGINS:
                    setPage(WINDOW_PLUGIN_PAGE_PLUGINS);
                    break;
                case WIDX_TAB_MODULES:
                    setPage(WINDOW_PLUGIN_PAGE_MODULES);
                    break;
                case WIDX_RELOAD_PLUGINS:
                    ReloadPlugins();
                    break;
                case WIDX_OPEN_FOLDER:
                    OpenPluginsFolder();
                    break;
            }
        }

        void onDraw(Drawing::RenderTarget& rt) override
        {
            drawWidgets(rt);

            // Draw Tab Icons
            auto screenCoordsTab1 = windowPos + ScreenCoordsXY{ widgets[WIDX_TAB_PLUGINS].left, widgets[WIDX_TAB_PLUGINS].top };
            GfxDrawSprite(rt, ImageId(SPR_TAB_GEARS_0), screenCoordsTab1);

            auto screenCoordsTab2 = windowPos + ScreenCoordsXY{ widgets[WIDX_TAB_MODULES].left, widgets[WIDX_TAB_MODULES].top };
            GfxDrawSprite(rt, ImageId(SPR_TAB_RIDES_ROLLER_COASTERS_0), screenCoordsTab2);

            const auto textColour = colours[1];
            auto drawLine = [&](int32_t x, int32_t y, std::string_view text) {
                drawText(rt, windowPos + ScreenCoordsXY{ static_cast<int16_t>(x), static_cast<int16_t>(y) }, text, { textColour });
            };

            if (page == WINDOW_PLUGIN_PAGE_PLUGINS)
            {
                DrawPluginsPage(rt, drawLine);
            }
            else if (page == WINDOW_PLUGIN_PAGE_MODULES)
            {
                DrawModulesPage(rt, drawLine);
            }

            if (!_statusMessage.empty())
            {
                drawLine(16, 276, "{GREEN}" + _statusMessage);
            }
        }

    private:
        template<typename DrawLineFunc>
        void DrawPluginsPage(Drawing::RenderTarget& rt, DrawLineFunc drawLine)
        {
#ifdef ENABLE_SCRIPTING
            auto& scriptEngine = GetContext()->GetScriptEngine();
            auto& plugins = scriptEngine.GetPlugins();

            if (plugins.empty())
            {
                drawLine(18, 68, "No external plugins currently loaded in your plugins directory.");
                drawLine(18, 92, "NeXTycoon multi-language engine supports JavaScript (.js), TypeScript (.ts),");
                drawLine(18, 114, "Lua (.lua), and native C-ABI dynamic libraries (.dll) with zero downtime.");
                drawLine(18, 136, "Plugins can add custom rides, cheats, automated park staff, and extensions.");
                drawLine(18, 168, "{LIGHTBLUE}How to install plugins:");
                drawLine(26, 190, "1. Click [Open Plugins Folder] below to open your user plugins directory.");
                drawLine(26, 210, "2. Drop any .js, .lua, or compiled .dll plugin file into that folder.");
                drawLine(26, 230, "3. Click [Reload Plugins] to instantly activate them with zero restart!");
            }
            else
            {
                drawLine(18, 66, "{LIGHTBLUE}Installed Script Plugins (" + std::to_string(plugins.size()) + " Active):");

                int32_t y = 88;
                for (size_t i = 0; i < plugins.size() && i < 7; i++)
                {
                    const auto& plugin = plugins[i];
                    if (plugin == nullptr)
                        continue;

                    const auto& meta = plugin->GetMetadata();
                    std::string title = meta.Name.empty() ? "Unnamed Plugin" : meta.Name;
                    if (!meta.Version.empty())
                    {
                        title += " (v" + meta.Version + ")";
                    }

                    std::string authorStr;
                    if (!meta.Authors.empty() && !meta.Authors[0].empty())
                    {
                        authorStr = " by " + meta.Authors[0];
                    }

                    std::string langBadge;
                    switch (plugin->GetLanguage())
                    {
                        case Scripting::PluginLanguage::javascript:
                            langBadge = "{LIGHTBLUE}[JS]";
                            break;
                        case Scripting::PluginLanguage::typescript:
                            langBadge = "{LIGHTBLUE}[TS]";
                            break;
                        case Scripting::PluginLanguage::lua:
                            langBadge = "{PURPLE}[Lua]";
                            break;
                        case Scripting::PluginLanguage::python:
                            langBadge = "{YELLOW}[Python]";
                            break;
                        case Scripting::PluginLanguage::nativeLib:
                            langBadge = "{ORANGE}[Native DLL]";
                            break;
                        default:
                            langBadge = "{GREY}[Plugin]";
                            break;
                    }

                    drawLine(20, y, "• " + langBadge + " {BLACK}" + title + authorStr);
                    drawLine(440, y, "{GREEN}[ACTIVE]");
                    y += 24;
                }
            }
#else
            drawLine(18, 68, "{GREY}Scripting engine is disabled in this compilation build.");
#endif
        }

        template<typename DrawLineFunc>
        void DrawModulesPage(Drawing::RenderTarget& rt, DrawLineFunc drawLine)
        {
            drawLine(18, 66, "{LIGHTBLUE}NeXTycoon Universal Tycoon Modular Architecture:");

            // Core 1
            drawLine(20, 88,  "• RollerCoaster Tycoon 2 Core Engine");
            drawLine(360, 88, "{GREEN}[ACTIVE / CORE]");
            drawLine(32, 102, "{GREY}Full simulation loop, park economics, guest AI, ride mechanics.");

            // Core 2
            drawLine(20, 118, "• Direct3D 11 Hardware Presentation Engine");
            drawLine(360, 118, "{GREEN}[ACTIVE / D3D11]");
            drawLine(32, 132, "{GREY}High-performance DirectX 11 pipeline with FL 11_1 and unconstrained FPS.");

            // Core 3
            drawLine(20, 148, "• NeXTycoon Multi-Language Scripting Engine");
            drawLine(360, 148, "{GREEN}[ACTIVE / LUA+C-ABI]");
            drawLine(32, 162, "{GREY}Embedded Lua 5.4.7 sandbox, QuickJS runtime, and dynamic native DLL loader.");

            // Core 4
            drawLine(20, 178, "• Locomotion Transport Module Bridge");
            drawLine(360, 178, "{YELLOW}[STANDBY / BRIDGE]");
            drawLine(32, 192, "{GREY}Locomotion vehicle specs, track components, and sprite parser bridge.");

            // Core 5
            drawLine(20, 208, "• RollerCoaster Tycoon 1 Direct Importer");
            drawLine(360, 208, "{GREEN}[ACTIVE / READY]");
            drawLine(32, 222, "{GREY}Direct SV4/SC4/CSG loader for original RCT1 parks and scenarios.");

            // Core 6
            drawLine(20, 238, "• Post-Processing Pixel Shader FX");
            drawLine(360, 238, "{GREEN}[ACTIVE / SHADERS]");
            drawLine(32, 252, "{GREY}Crisp Nearest, Bilinear, Vibrant HDR Modern, and Retro CRT Scanlines.");
        }

        void ReloadPlugins()
        {
#ifdef ENABLE_SCRIPTING
            GetContext()->GetScriptEngine().StopUnloadRegisterAllPlugins();
            _statusMessage = "All plugins and scripts reloaded successfully!";
#else
            _statusMessage = "Scripting engine not available.";
#endif
            invalidate();
        }

        void OpenPluginsFolder()
        {
            auto dir = GetContext()->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::plugins);
            GetContext()->GetUiContext().OpenFolder(dir);
            _statusMessage = "Opened plugins directory.";
            invalidate();
        }
    };

    WindowBase* PluginManagerOpen()
    {
        auto* windowMgr = GetWindowManager();
        return windowMgr->FocusOrCreate<PluginManagerWindow>(
            WindowClass::pluginManager, kWindowSize, WindowFlag::centreScreen);
    }
} // namespace OpenRCT2::Ui::Windows

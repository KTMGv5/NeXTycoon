/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef ENABLE_SCRIPTING

    #include "Plugin.h"

    #include "../Context.h"
    #include "../Diagnostic.h"
    #include "../core/File.h"
    #include "../core/Path.hpp"
    #include "../core/String.hpp"
    #include "ScriptEngine.h"

using namespace OpenRCT2::Scripting;

Plugin::Plugin(std::string_view path)
    : _path(path)
{
    DetectLanguage();
}

void Plugin::DetectLanguage()
{
    if (_path.empty())
    {
        _language = PluginLanguage::javascript;
        return;
    }

    auto ext = Path::GetExtension(_path);
    if (String::iequals(ext, ".ts"))
    {
        _language = PluginLanguage::typescript;
    }
    else if (String::iequals(ext, ".lua"))
    {
        _language = PluginLanguage::lua;
    }
    else if (String::iequals(ext, ".py"))
    {
        _language = PluginLanguage::python;
    }
    else if (String::iequals(ext, ".dll") || String::iequals(ext, ".so") || String::iequals(ext, ".dylib"))
    {
        _language = PluginLanguage::nativeLib;
    }
    else
    {
        _language = PluginLanguage::javascript;
    }
    _metadata.Language = _language;
}

void Plugin::LoadNonJsMetadata()
{
    _metadata.Name = Path::GetFileNameWithoutExtension(_path);
    _metadata.Version = "1.0.0";
    _metadata.Type = PluginType::intransient;
    _metadata.Language = _language;

    // Parse simple key = "value" pairs from comments/script content if available
    if (!_code.empty())
    {
        std::string_view sv = _code;
        size_t pos = 0;
        while (pos < sv.size())
        {
            size_t endOfLine = sv.find('\n', pos);
            if (endOfLine == std::string_view::npos)
                endOfLine = sv.size();

            std::string_view line = sv.substr(pos, endOfLine - pos);
            pos = endOfLine + 1;

            auto namePos = line.find("name");
            auto quoteStart = line.find('"', namePos != std::string_view::npos ? namePos : 0);
            if (quoteStart != std::string_view::npos)
            {
                auto quoteEnd = line.find('"', quoteStart + 1);
                if (quoteEnd != std::string_view::npos)
                {
                    if (line.find("name") != std::string_view::npos && line.find("=") != std::string_view::npos)
                    {
                        _metadata.Name = std::string(line.substr(quoteStart + 1, quoteEnd - quoteStart - 1));
                    }
                    else if (line.find("version") != std::string_view::npos && line.find("=") != std::string_view::npos)
                    {
                        _metadata.Version = std::string(line.substr(quoteStart + 1, quoteEnd - quoteStart - 1));
                    }
                    else if (line.find("author") != std::string_view::npos && line.find("=") != std::string_view::npos)
                    {
                        _metadata.Authors.push_back(std::string(line.substr(quoteStart + 1, quoteEnd - quoteStart - 1)));
                    }
                }
            }
        }
    }
}

void Plugin::SetCode(std::string_view code)
{
    _code = code;
}

static JSValue registerPlugin(JSContext* ctx, JSValue thisVal, int argc, JSValue* argv)
{
    Plugin* plugin = static_cast<Plugin*>(JS_GetContextOpaque(ctx));
    plugin->SetMetadata(argv[0]);
    return JS_UNDEFINED;
}

void Plugin::Load()
{
    if (_context)
    {
        JS_FreeContext(_context);
        _context = nullptr;
    }

    if (!_path.empty())
    {
        LoadCodeFromFile();
    }

    if (_language != PluginLanguage::javascript && _language != PluginLanguage::typescript)
    {
        LoadNonJsMetadata();
        _hasLoaded = true;
        return;
    }

    auto& scriptEngine = OpenRCT2::GetContext()->GetScriptEngine();
    _context = scriptEngine.CreateContext();
    JS_SetContextOpaque(_context, this);

    JSValue registerFunc = JS_NewCFunction(_context, registerPlugin, "registerPlugin", 1);
    JSValue glb = JS_GetGlobalObject(_context);
    JS_SetPropertyStr(_context, glb, "registerPlugin", registerFunc);
    JS_FreeValue(_context, glb);

    JSValue res = JS_Eval(_context, _code.c_str(), _code.length(), _path.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(res))
    {
        JSValue exceptionVal = JS_GetException(_context);
        std::string details = Stringify(_context, exceptionVal);
        JS_FreeValue(_context, exceptionVal);
        JS_FreeValue(_context, res);
        JS_FreeContext(_context);
        throw std::runtime_error("Failed to load plug-in script: " + _path + "\n" + details);
    }
    JS_FreeValue(_context, res);

    _hasLoaded = true;
}

void Plugin::Start()
{
    if (!_hasLoaded)
    {
        throw std::runtime_error("Plugin has not been loaded.");
    }

    if (_language != PluginLanguage::javascript && _language != PluginLanguage::typescript)
    {
        _hasStarted = true;
        return;
    }

    const JSValue mainFunc = _metadata.Main.callback;
    if (!JS_IsFunction(_context, mainFunc))
    {
        throw std::runtime_error("No main function specified.");
    }

    _hasStarted = true;

    const JSValue res = JS_Call(_context, mainFunc, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(res))
    {
        JSValue exceptionVal = JS_GetException(_context);
        std::string details = Stringify(_context, exceptionVal);
        JS_FreeValue(_context, exceptionVal);
        JS_FreeValue(_context, res);
        throw std::runtime_error("[" + _metadata.Name + "]\n" + details);
    }
    JS_FreeValue(_context, res);
}

void Plugin::StopBegin()
{
    _isStopping = true;
}

void Plugin::StopEnd()
{
    _isStopping = false;
    _hasStarted = false;
}

void Plugin::Unload()
{
    if (_language != PluginLanguage::javascript && _language != PluginLanguage::typescript)
    {
        _hasLoaded = false;
        return;
    }

    if (!_context)
    {
        throw std::runtime_error("Plugin is not loaded");
    }

    JS_FreeContext(_context);
    _context = nullptr;

    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105937, fixed in GCC13
    #if defined(__GNUC__) && !defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    #endif
    _metadata.Main = {};
    #if defined(__GNUC__) && !defined(__clang__)
        #pragma GCC diagnostic pop
    #endif
    _hasLoaded = false;
}

void Plugin::LoadCodeFromFile()
{
    _code = File::ReadAllText(_path);
}

std::string Plugin::TryGetString(const JSValue value, const char* property, const std::string& message) const
{
    const JSValue res = JS_GetPropertyStr(_context, value, property);
    if (!JS_IsString(res))
    {
        JS_FreeValue(_context, res);
        throw std::runtime_error(message);
    }
    std::string str = JSToStdString(_context, res);
    JS_FreeValue(_context, res);
    return str;
}

void Plugin::SetMetadata(const JSValue obj)
{
    PluginMetadata metadata{};
    if (JS_IsObject(obj))
    {
        metadata.Name = TryGetString(obj, "name", "Plugin name not specified.");
        metadata.Version = TryGetString(obj, "version", "Plugin version not specified.");
        metadata.Type = ParsePluginType(TryGetString(obj, "type", "Plugin type not specified."));

        const JSValue licence = JS_GetPropertyStr(_context, obj, "licence");
        int64_t licenseLen = -1;
        if (!JS_IsString(licence) || JS_GetLength(_context, licence, &licenseLen) == -1 || licenseLen == 0)
        {
            LOG_ERROR("Plugin %s does not specify a licence", _metadata.Name.c_str());
        }
        JS_FreeValue(_context, licence);

        const JSValue minApiVersion = JS_GetPropertyStr(_context, obj, "minApiVersion");
        if (JS_IsNumber(minApiVersion))
        {
            int64_t val = -1;
            JS_ToInt64(_context, &val, minApiVersion);
            metadata.MinApiVersion = val;
        }
        JS_FreeValue(_context, minApiVersion);

        const JSValue targetApiVersion = JS_GetPropertyStr(_context, obj, "targetApiVersion");
        if (JS_IsNumber(targetApiVersion))
        {
            int64_t val = -1;
            JS_ToInt64(_context, &val, targetApiVersion);
            metadata.TargetApiVersion = val;
        }
        else
        {
            LOG_ERROR(
                u8"Plug-in “%s” does not specify a target API version or specifies it incorrectly. Emulating deprecated APIs.",
                metadata.Name.c_str());
        }
        JS_FreeValue(_context, targetApiVersion);

        const JSValue authors = JS_GetPropertyStr(_context, obj, "authors");
        if (JS_IsArray(authors))
        {
            JSIterateArray(_context, authors, [&metadata](JSContext* ctx2, JSValue val) {
                if (JS_IsString(val))
                    metadata.Authors.emplace_back(JSToStdString(ctx2, val));
            });
        }
        else if (JS_IsString(authors))
        {
            metadata.Authors = { JSToStdString(_context, authors) };
        }
        JS_FreeValue(_context, authors);

        metadata.Main = JSToCallback(_context, obj, "main");
    }
    _metadata = metadata;
}

PluginType Plugin::ParsePluginType(std::string_view type)
{
    if (type == "local")
        return PluginType::local;
    if (type == "remote")
        return PluginType::remote;
    if (type == "intransient")
        return PluginType::intransient;
    throw std::invalid_argument("Unknown plugin type.");
}

int32_t Plugin::GetTargetAPIVersion() const
{
    if (_metadata.TargetApiVersion)
        return *_metadata.TargetApiVersion;

    // If not specified, default to 33 since that is the API version from before 'targetAPIVersion' was introduced.
    return 33;
}

bool Plugin::IsTransient() const
{
    return _metadata.Type != PluginType::intransient;
}

#endif

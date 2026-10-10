/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

// Windows.h needs to be included first
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Enable visual styles
#pragma comment(                                                                                                               \
    linker,                                                                                                                    \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Then the rest
#include <algorithm>
#include <iterator>
#include <openrct2-ui/Ui.h>
#include <openrct2/core/EnumUtils.hpp>
#include <openrct2/core/String.hpp>
#include <string>
#include <vector>

#include <fcntl.h>
#include <io.h>

static std::vector<std::string> GetCommandLineArgs(int argc, wchar_t** argvW);

static void SetupConsoleAndRedirection()
{
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

    auto isRedirected = [](HANDLE h) {
        if (h == NULL || h == INVALID_HANDLE_VALUE)
            return false;
        DWORD fileType = GetFileType(h);
        return (fileType == FILE_TYPE_DISK || fileType == FILE_TYPE_PIPE);
    };

    bool stdOutRedirected = isRedirected(hStdOut);
    bool stdErrRedirected = isRedirected(hStdErr);
    bool stdInRedirected = isRedirected(hStdIn);

    if (stdOutRedirected)
    {
        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStdOut), _O_TEXT);
        if (fd != -1)
        {
            FILE* fp = _fdopen(fd, "w");
            if (fp != nullptr)
            {
                *stdout = *fp;
                setvbuf(stdout, nullptr, _IONBF, 0);
            }
        }
    }
    if (stdErrRedirected)
    {
        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStdErr), _O_TEXT);
        if (fd != -1)
        {
            FILE* fp = _fdopen(fd, "w");
            if (fp != nullptr)
            {
                *stderr = *fp;
                setvbuf(stderr, nullptr, _IONBF, 0);
            }
        }
    }
    if (stdInRedirected)
    {
        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStdIn), _O_TEXT);
        if (fd != -1)
        {
            FILE* fp = _fdopen(fd, "r");
            if (fp != nullptr)
            {
                *stdin = *fp;
            }
        }
    }

    if (!stdOutRedirected || !stdErrRedirected || !stdInRedirected)
    {
        if (AttachConsole(ATTACH_PARENT_PROCESS))
        {
            FILE* fp;
            if (!stdOutRedirected)
            {
                freopen_s(&fp, "CONOUT$", "w", stdout);
                setvbuf(stdout, nullptr, _IONBF, 0);
            }
            if (!stdErrRedirected)
            {
                freopen_s(&fp, "CONOUT$", "w", stderr);
                setvbuf(stderr, nullptr, _IONBF, 0);
            }
            if (!stdInRedirected)
            {
                freopen_s(&fp, "CONIN$", "r", stdin);
            }
        }
    }
}

/**
 * Windows GUI entry point for NeXTycoon (no console pop-up when launched).
 */
int WINAPI wWinMain(
    [[maybe_unused]] HINSTANCE hInstance,
    [[maybe_unused]] HINSTANCE hPrevInstance,
    [[maybe_unused]] PWSTR pCmdLine,
    [[maybe_unused]] int nCmdShow)
{
    SetupConsoleAndRedirection();

    int argc = __argc;
    wchar_t** argvW = __wargv;
    auto argvStrings = GetCommandLineArgs(argc, argvW);

    SetConsoleCP(EnumValue(OpenRCT2::CodePage::utf8));
    SetConsoleOutputCP(EnumValue(OpenRCT2::CodePage::utf8));

    std::vector<const char*> argv;
    std::transform(
        argvStrings.begin(), argvStrings.end(), std::back_inserter(argv), [](const auto& string) { return string.c_str(); });

    // Ensure that argv[argc] == nullptr, as mandated by the standard
    argv.push_back(nullptr);
    int result = NormalisedMain(argc, argv.data());
    fflush(stdout);
    fflush(stderr);
    return result;
}

/**
 * Fallback console entry point.
 */
int wmain([[maybe_unused]] int argc, [[maybe_unused]] wchar_t** argvW, [[maybe_unused]] wchar_t* envp)
{
    return wWinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_SHOW);
}

static std::vector<std::string> GetCommandLineArgs(int argc, wchar_t** argvW)
{
    // Allocate UTF-8 strings
    std::vector<std::string> argv;
    for (int i = 0; i < argc; i++)
    {
        argv.push_back(OpenRCT2::String::toUtf8(argvW[i]));
    }
    return argv;
}

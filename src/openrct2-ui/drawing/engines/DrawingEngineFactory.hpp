/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <memory>
#include <openrct2/Diagnostic.h>
#include <openrct2/core/Guard.hpp>
#include <openrct2/drawing/IDrawingEngine.h>

namespace OpenRCT2::Ui
{
    struct IUiContext;

    [[nodiscard]] std::unique_ptr<Drawing::IDrawingEngine> CreateHardwareDisplayDrawingEngine(IUiContext& uiContext);
#ifndef DISABLE_OPENGL
    [[nodiscard]] std::unique_ptr<Drawing::IDrawingEngine> CreateOpenGLDrawingEngine(IUiContext& uiContext);
#endif
#if defined(_WIN32)
    [[nodiscard]] std::unique_ptr<Drawing::IDrawingEngine> CreateD3D9DrawingEngine(IUiContext& uiContext);
    [[nodiscard]] std::unique_ptr<Drawing::IDrawingEngine> CreateD3D11DrawingEngine(IUiContext& uiContext);
#endif

    class DrawingEngineFactory final : public Drawing::IDrawingEngineFactory
    {
    public:
        [[nodiscard]] std::unique_ptr<Drawing::IDrawingEngine> Create(DrawingEngine type, IUiContext& uiContext) override
        {
            switch (type)
            {
                case DrawingEngine::softwareWithHardwareDisplay:
                    return CreateHardwareDisplayDrawingEngine(uiContext);
#ifndef DISABLE_OPENGL
                case DrawingEngine::openGL:
#if defined(_WIN32)
                    // OpenGL is eliminated on Windows; route to highest available DirectX (Direct3D 11)
                    return Create(DrawingEngine::direct3D11, uiContext);
#else
                    return CreateOpenGLDrawingEngine(uiContext);
#endif
#endif
#if defined(_WIN32)
                case DrawingEngine::direct3D9:
                    return CreateD3D9DrawingEngine(uiContext);
                case DrawingEngine::direct3D11:
                {
                    try
                    {
                        auto engine = CreateD3D11DrawingEngine(uiContext);
                        if (engine != nullptr)
                        {
                            return engine;
                        }
                    }
                    catch (const std::exception& ex)
                    {
                        LOG_WARNING("Direct3D 11 initialization failed (%s); falling back to Direct3D 9", ex.what());
                    }
                    catch (...)
                    {
                        LOG_WARNING("Direct3D 11 initialization failed; falling back to Direct3D 9");
                    }
                    return CreateD3D9DrawingEngine(uiContext);
                }
#endif
                default:
                    Guard::Fail("Unknown renderer: %u", static_cast<uint32_t>(type));
                    return nullptr;
            }
        }
    };
} // namespace OpenRCT2::Ui

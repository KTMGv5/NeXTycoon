/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VulkanDrawingEngine.h"
#include "DrawingEngineFactory.hpp"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef CreateWindow
#undef small
#undef min
#undef max
#include <vulkan/vulkan_win32.h>
#endif
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <openrct2/Diagnostic.h>
#include <openrct2/Game.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Guard.hpp>
#include <openrct2/core/String.hpp>
#include <openrct2/drawing/Drawing.Screen.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/IDrawingEngine.h>
#include <openrct2/drawing/PaletteType.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Window.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Weather.h>

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
using namespace OpenRCT2::Ui;

namespace
{
    struct VulkanProcs
    {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
        PFN_vkCreateInstance vkCreateInstance = nullptr;
        PFN_vkDestroyInstance vkDestroyInstance = nullptr;
        PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
        PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
        PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = nullptr;
        PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties = nullptr;
        PFN_vkCreateDevice vkCreateDevice = nullptr;
        PFN_vkDestroyDevice vkDestroyDevice = nullptr;
        PFN_vkGetDeviceQueue vkGetDeviceQueue = nullptr;
        PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;

        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
        PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
        PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;

        PFN_vkCreateCommandPool vkCreateCommandPool = nullptr;
        PFN_vkDestroyCommandPool vkDestroyCommandPool = nullptr;
        PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = nullptr;
        PFN_vkFreeCommandBuffers vkFreeCommandBuffers = nullptr;
        PFN_vkBeginCommandBuffer vkBeginCommandBuffer = nullptr;
        PFN_vkEndCommandBuffer vkEndCommandBuffer = nullptr;
        PFN_vkResetCommandBuffer vkResetCommandBuffer = nullptr;
        PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = nullptr;
        PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = nullptr;
        PFN_vkCmdCopyImage vkCmdCopyImage = nullptr;
        PFN_vkCmdBlitImage vkCmdBlitImage = nullptr;
        PFN_vkQueueSubmit vkQueueSubmit = nullptr;
        PFN_vkQueueWaitIdle vkQueueWaitIdle = nullptr;
        PFN_vkDeviceWaitIdle vkDeviceWaitIdle = nullptr;

        PFN_vkCreateImage vkCreateImage = nullptr;
        PFN_vkDestroyImage vkDestroyImage = nullptr;
        PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = nullptr;
        PFN_vkAllocateMemory vkAllocateMemory = nullptr;
        PFN_vkFreeMemory vkFreeMemory = nullptr;
        PFN_vkBindImageMemory vkBindImageMemory = nullptr;
        PFN_vkCreateBuffer vkCreateBuffer = nullptr;
        PFN_vkDestroyBuffer vkDestroyBuffer = nullptr;
        PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = nullptr;
        PFN_vkBindBufferMemory vkBindBufferMemory = nullptr;
        PFN_vkMapMemory vkMapMemory = nullptr;
        PFN_vkUnmapMemory vkUnmapMemory = nullptr;
        PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = nullptr;

        PFN_vkCreateFence vkCreateFence = nullptr;
        PFN_vkDestroyFence vkDestroyFence = nullptr;
        PFN_vkWaitForFences vkWaitForFences = nullptr;
        PFN_vkResetFences vkResetFences = nullptr;
        PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
        PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;
    };
} // namespace

class VulkanDrawingEngine final : public X8DrawingEngine
{
private:
    IUiContext& _uiContext;
    SDL_Window* _window = nullptr;

    VulkanProcs _vk;

    VkInstance _instance = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    uint32_t _queueFamilyIndex = 0;
    VkQueue _queue = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties _memProperties{};

    VkFormat _swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR _swapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D _swapchainExtent = { 0, 0 };
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> _swapchainImages;

    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;

    VkSemaphore _imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore _renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence _inFlightFence = VK_NULL_HANDLE;

    // Staging buffer for CPU -> GPU pixel transfer
    VkBuffer _stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _stagingMemory = VK_NULL_HANDLE;
    void* _stagingMappedPtr = nullptr;
    size_t _stagingBufferSize = 0;

    // Intermediate GPU image for blitting / scaling
    VkImage _gameImage = VK_NULL_HANDLE;
    VkDeviceMemory _gameImageMemory = VK_NULL_HANDLE;
    uint32_t _gameImageWidth = 0;
    uint32_t _gameImageHeight = 0;

    bool _supportsBlit = true;
    bool _useVsync = false;
    uint32_t _paletteHWMapped[256] = { 0 };
    void* _vulkanLibHandle = nullptr;

    void GetWindowDrawableSize(int32_t* outW, int32_t* outH)
    {
        int32_t w = 0, h = 0;
#if defined(_WIN32)
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(_window, &wmInfo))
        {
            RECT rc;
            if (GetClientRect(wmInfo.info.win.window, &rc))
            {
                w = rc.right - rc.left;
                h = rc.bottom - rc.top;
            }
        }
#endif
        if (w <= 0 || h <= 0)
        {
            SDL_GetWindowSize(_window, &w, &h);
        }
        *outW = w;
        *outH = h;
    }

public:
    explicit VulkanDrawingEngine(IUiContext& uiContext)
        : X8DrawingEngine(uiContext)
        , _uiContext(uiContext)
    {
        _window = static_cast<SDL_Window*>(_uiContext.GetWindow());
        _useVsync = Config::Get().general.useVSync;
    }

    ~VulkanDrawingEngine() override
    {
        CleanupVulkan();
    }

    void Initialise() override
    {
        X8DrawingEngine::Initialise();

        LoadVulkanFunctions();
        CreateInstance();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();
        CreateSyncObjects();

        int32_t dw = 0, dh = 0;
        GetWindowDrawableSize(&dw, &dh);
        CreateSwapChain(static_cast<uint32_t>(dw), static_cast<uint32_t>(dh));
        EnsureStagingAndGameImage(_width, _height);

        VkPhysicalDeviceProperties devProps{};
        _vk.vkGetPhysicalDeviceProperties(_physicalDevice, &devProps);
        LOG_INFO("Vulkan: Initialised hardware drawing engine on '%s' (swapchain %ux%u)", devProps.deviceName, _swapchainExtent.width, _swapchainExtent.height);
    }

    void Resize(uint32_t width, uint32_t height) override
    {
        if (_device != VK_NULL_HANDLE)
        {
            _vk.vkDeviceWaitIdle(_device);
        }

        X8DrawingEngine::Resize(width, height);

        int32_t dw = 0, dh = 0;
        GetWindowDrawableSize(&dw, &dh);

        if (dw > 0 && dh > 0)
        {
            CreateSwapChain(static_cast<uint32_t>(dw), static_cast<uint32_t>(dh));
        }
        EnsureStagingAndGameImage(_width, _height);
    }

    void SetPalette(const GamePalette& palette) override
    {
        for (size_t i = 0; i < 256; i++)
        {
            const auto& entry = palette[i];
            if (_swapchainFormat == VK_FORMAT_B8G8R8A8_UNORM || _swapchainFormat == VK_FORMAT_B8G8R8A8_SRGB)
            {
                // BGRA format
                _paletteHWMapped[i] = (static_cast<uint32_t>(0xFF) << 24) |
                                      (static_cast<uint32_t>(entry.red) << 16) |
                                      (static_cast<uint32_t>(entry.green) << 8) |
                                      (static_cast<uint32_t>(entry.blue));
            }
            else
            {
                // RGBA format
                _paletteHWMapped[i] = (static_cast<uint32_t>(0xFF) << 24) |
                                      (static_cast<uint32_t>(entry.blue) << 16) |
                                      (static_cast<uint32_t>(entry.green) << 8) |
                                      (static_cast<uint32_t>(entry.red));
            }
        }
    }

    void SetVSync(bool vsync) override
    {
        if (_useVsync != vsync)
        {
            _useVsync = vsync;
            if (_device != VK_NULL_HANDLE && _swapchainExtent.width > 0 && _swapchainExtent.height > 0)
            {
                _vk.vkDeviceWaitIdle(_device);
                CreateSwapChain(_swapchainExtent.width, _swapchainExtent.height);
            }
        }
    }

    void BeginDraw() override
    {
        X8DrawingEngine::BeginDraw();
    }

    void EndDraw() override
    {
        X8DrawingEngine::EndDraw();
        Display();
    }

    void PaintWindows() override
    {
        if (Weather::hasWeatherEffect() || gPaintForceRedraw)
        {
            WindowUpdateAllViewports();
            WindowDrawAll(_mainRT, 0, 0, static_cast<int32_t>(_width), static_cast<int32_t>(_height));
        }
        else
        {
            X8DrawingEngine::PaintWindows();
        }
    }

private:
    void LoadVulkanFunctions()
    {
#if defined(_WIN32)
        HMODULE hVk = LoadLibraryA("vulkan-1.dll");
        if (hVk == nullptr)
        {
            throw std::runtime_error("vulkan-1.dll not found on system. Please ensure your graphics drivers are up to date.");
        }
        _vulkanLibHandle = static_cast<void*>(hVk);
        _vk.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(hVk, "vkGetInstanceProcAddr"));
#elif defined(__APPLE__)
        void* hVk = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
        if (hVk == nullptr) hVk = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
        if (hVk != nullptr)
        {
            _vulkanLibHandle = hVk;
            _vk.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(hVk, "vkGetInstanceProcAddr"));
        }
        else if (SDL_Vulkan_LoadLibrary(nullptr) == 0)
        {
            _vk.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
        }
        else
        {
            throw std::runtime_error("Vulkan/MoltenVK runtime library not found.");
        }
#else
        void* hVk = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
        if (hVk == nullptr) hVk = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        if (hVk != nullptr)
        {
            _vulkanLibHandle = hVk;
            _vk.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(hVk, "vkGetInstanceProcAddr"));
        }
        else if (SDL_Vulkan_LoadLibrary(nullptr) == 0)
        {
            _vk.vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
        }
        else
        {
            throw std::runtime_error("libvulkan.so not found on system.");
        }
#endif

        if (_vk.vkGetInstanceProcAddr == nullptr)
        {
            throw std::runtime_error("Failed to load vkGetInstanceProcAddr entrypoint.");
        }

        _vk.vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(_vk.vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
        if (_vk.vkCreateInstance == nullptr)
        {
            throw std::runtime_error("Failed to load vkCreateInstance entrypoint.");
        }
    }

    void LoadInstanceFunctions()
    {
        #define LOAD_INST(fn) _vk.fn = reinterpret_cast<PFN_##fn>(_vk.vkGetInstanceProcAddr(_instance, #fn))
        LOAD_INST(vkDestroyInstance);
        LOAD_INST(vkEnumeratePhysicalDevices);
        LOAD_INST(vkGetPhysicalDeviceProperties);
        LOAD_INST(vkGetPhysicalDeviceQueueFamilyProperties);
        LOAD_INST(vkGetPhysicalDeviceMemoryProperties);
        LOAD_INST(vkGetPhysicalDeviceFeatures);
        LOAD_INST(vkGetPhysicalDeviceFormatProperties);
        LOAD_INST(vkCreateDevice);
        LOAD_INST(vkDestroyDevice);
        LOAD_INST(vkGetDeviceQueue);
        LOAD_INST(vkGetDeviceProcAddr);
        LOAD_INST(vkDestroySurfaceKHR);
        LOAD_INST(vkGetPhysicalDeviceSurfaceSupportKHR);
        LOAD_INST(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        LOAD_INST(vkGetPhysicalDeviceSurfaceFormatsKHR);
        LOAD_INST(vkGetPhysicalDeviceSurfacePresentModesKHR);
        #undef LOAD_INST
    }

    void LoadDeviceFunctions()
    {
        #define LOAD_DEV(fn) _vk.fn = reinterpret_cast<PFN_##fn>(_vk.vkGetDeviceProcAddr(_device, #fn))
        LOAD_DEV(vkCreateSwapchainKHR);
        LOAD_DEV(vkDestroySwapchainKHR);
        LOAD_DEV(vkGetSwapchainImagesKHR);
        LOAD_DEV(vkAcquireNextImageKHR);
        LOAD_DEV(vkQueuePresentKHR);
        LOAD_DEV(vkCreateCommandPool);
        LOAD_DEV(vkDestroyCommandPool);
        LOAD_DEV(vkAllocateCommandBuffers);
        LOAD_DEV(vkFreeCommandBuffers);
        LOAD_DEV(vkBeginCommandBuffer);
        LOAD_DEV(vkEndCommandBuffer);
        LOAD_DEV(vkResetCommandBuffer);
        LOAD_DEV(vkCmdPipelineBarrier);
        LOAD_DEV(vkCmdCopyBufferToImage);
        LOAD_DEV(vkCmdCopyImage);
        LOAD_DEV(vkCmdBlitImage);
        LOAD_DEV(vkQueueSubmit);
        LOAD_DEV(vkQueueWaitIdle);
        LOAD_DEV(vkDeviceWaitIdle);
        LOAD_DEV(vkCreateImage);
        LOAD_DEV(vkDestroyImage);
        LOAD_DEV(vkGetImageMemoryRequirements);
        LOAD_DEV(vkAllocateMemory);
        LOAD_DEV(vkFreeMemory);
        LOAD_DEV(vkBindImageMemory);
        LOAD_DEV(vkCreateBuffer);
        LOAD_DEV(vkDestroyBuffer);
        LOAD_DEV(vkGetBufferMemoryRequirements);
        LOAD_DEV(vkBindBufferMemory);
        LOAD_DEV(vkMapMemory);
        LOAD_DEV(vkUnmapMemory);
        LOAD_DEV(vkFlushMappedMemoryRanges);
        LOAD_DEV(vkCreateFence);
        LOAD_DEV(vkDestroyFence);
        LOAD_DEV(vkWaitForFences);
        LOAD_DEV(vkResetFences);
        LOAD_DEV(vkCreateSemaphore);
        LOAD_DEV(vkDestroySemaphore);
        #undef LOAD_DEV
    }

    void CreateInstance()
    {
        std::vector<const char*> extensions;
#if defined(_WIN32)
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
        unsigned int extCount = 0;
        if (SDL_Vulkan_GetInstanceExtensions(_window, &extCount, nullptr) && extCount > 0)
        {
            extensions.resize(extCount);
            SDL_Vulkan_GetInstanceExtensions(_window, &extCount, extensions.data());
        }
        else
        {
            extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(__APPLE__)
            extensions.push_back("VK_EXT_metal_surface");
#else
            extensions.push_back("VK_KHR_xlib_surface");
#endif
        }
#endif

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "NeXTycoon";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "NeXTycoonEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkResult res = _vk.vkCreateInstance(&createInfo, nullptr, &_instance);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error(String::stdFormat("vkCreateInstance failed with error %d", res));
        }

        LoadInstanceFunctions();
    }

    void CreateSurface()
    {
#if defined(_WIN32)
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (!SDL_GetWindowWMInfo(_window, &wmInfo))
        {
            throw std::runtime_error(String::stdFormat("SDL_GetWindowWMInfo failed: %s", SDL_GetError()));
        }
        HWND hwnd = wmInfo.info.win.window;

        auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(_vk.vkGetInstanceProcAddr(_instance, "vkCreateWin32SurfaceKHR"));
        if (vkCreateWin32SurfaceKHR == nullptr)
        {
            throw std::runtime_error("vkCreateWin32SurfaceKHR entrypoint not found");
        }

        VkWin32SurfaceCreateInfoKHR sci{};
        sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        sci.hinstance = GetModuleHandle(nullptr);
        sci.hwnd = hwnd;

        VkResult res = vkCreateWin32SurfaceKHR(_instance, &sci, nullptr, &_surface);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error(String::stdFormat("vkCreateWin32SurfaceKHR failed with error %d", res));
        }
#else
        if (!SDL_Vulkan_CreateSurface(_window, _instance, &_surface))
        {
            throw std::runtime_error(String::stdFormat("SDL_Vulkan_CreateSurface failed: %s", SDL_GetError()));
        }
#endif
    }

    void PickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        _vk.vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("No Vulkan physical devices found");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        _vk.vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        uint32_t bestQueueFamily = UINT32_MAX;
        bool bestIsDiscrete = false;

        for (const auto& dev : devices)
        {
            uint32_t queueFamilyCount = 0;
            _vk.vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            _vk.vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

            for (uint32_t i = 0; i < queueFamilyCount; i++)
            {
                VkBool32 presentSupport = VK_FALSE;
                _vk.vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, _surface, &presentSupport);

                if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
                {
                    VkPhysicalDeviceProperties props{};
                    _vk.vkGetPhysicalDeviceProperties(dev, &props);
                    bool isDiscrete = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

                    if (bestDevice == VK_NULL_HANDLE || (!bestIsDiscrete && isDiscrete))
                    {
                        bestDevice = dev;
                        bestQueueFamily = i;
                        bestIsDiscrete = isDiscrete;
                    }
                    break;
                }
            }
        }

        if (bestDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("No suitable Vulkan physical device with graphics & presentation support found");
        }

        _physicalDevice = bestDevice;
        _queueFamilyIndex = bestQueueFamily;
        _vk.vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_memProperties);

        VkFormatProperties formatProps{};
        _vk.vkGetPhysicalDeviceFormatProperties(_physicalDevice, VK_FORMAT_B8G8R8A8_UNORM, &formatProps);
        _supportsBlit = (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) &&
                        (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
    }

    void CreateLogicalDevice()
    {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = _queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        const char* deviceExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 1;
        createInfo.ppEnabledExtensionNames = deviceExtensions;

        VkResult res = _vk.vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error(String::stdFormat("vkCreateDevice failed with error %d", res));
        }

        LoadDeviceFunctions();
        _vk.vkGetDeviceQueue(_device, _queueFamilyIndex, 0, &_queue);
    }

    void CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = _queueFamilyIndex;

        VkResult res = _vk.vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error(String::stdFormat("vkCreateCommandPool failed with error %d", res));
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        res = _vk.vkAllocateCommandBuffers(_device, &allocInfo, &_commandBuffer);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error(String::stdFormat("vkAllocateCommandBuffers failed with error %d", res));
        }
    }

    void CreateSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (_vk.vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphore) != VK_SUCCESS ||
            _vk.vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphore) != VK_SUCCESS ||
            _vk.vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan synchronization objects");
        }
    }

    void CreateSwapChain(uint32_t width, uint32_t height)
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        _vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);

        uint32_t formatCount = 0;
        _vk.vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        _vk.vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, formats.data());

        VkSurfaceFormatKHR surfaceFormat = formats[0];
        for (const auto& fmt : formats)
        {
            if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surfaceFormat = fmt;
                break;
            }
        }
        _swapchainFormat = surfaceFormat.format;
        _swapchainColorSpace = surfaceFormat.colorSpace;

        uint32_t presentModeCount = 0;
        _vk.vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        _vk.vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, presentModes.data());

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Guaranteed by Vulkan standard
        if (!_useVsync)
        {
            for (const auto& mode : presentModes)
            {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR && presentMode != VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }
        }

        VkExtent2D extent = { width, height };
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            extent = capabilities.currentExtent;
        }
        else
        {
            extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        _swapchainExtent = extent;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = _swapchain;

        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        VkResult res = _vk.vkCreateSwapchainKHR(_device, &createInfo, nullptr, &newSwapchain);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error(String::stdFormat("vkCreateSwapchainKHR failed with error %d", res));
        }

        if (_swapchain != VK_NULL_HANDLE)
        {
            _vk.vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        }
        _swapchain = newSwapchain;

        uint32_t finalImageCount = 0;
        _vk.vkGetSwapchainImagesKHR(_device, _swapchain, &finalImageCount, nullptr);
        _swapchainImages.resize(finalImageCount);
        _vk.vkGetSwapchainImagesKHR(_device, _swapchain, &finalImageCount, _swapchainImages.data());
    }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < _memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (_memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable Vulkan memory type");
    }

    void EnsureStagingAndGameImage(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        size_t requiredSize = width * height * 4;
        if (_stagingBuffer == VK_NULL_HANDLE || _stagingBufferSize < requiredSize)
        {
            if (_stagingMappedPtr != nullptr)
            {
                _vk.vkUnmapMemory(_device, _stagingMemory);
                _stagingMappedPtr = nullptr;
            }
            if (_stagingBuffer != VK_NULL_HANDLE)
            {
                _vk.vkDestroyBuffer(_device, _stagingBuffer, nullptr);
                _stagingBuffer = VK_NULL_HANDLE;
            }
            if (_stagingMemory != VK_NULL_HANDLE)
            {
                _vk.vkFreeMemory(_device, _stagingMemory, nullptr);
                _stagingMemory = VK_NULL_HANDLE;
            }

            _stagingBufferSize = requiredSize;

            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = _stagingBufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            _vk.vkCreateBuffer(_device, &bufferInfo, nullptr, &_stagingBuffer);

            VkMemoryRequirements memReq{};
            _vk.vkGetBufferMemoryRequirements(_device, _stagingBuffer, &memReq);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            _vk.vkAllocateMemory(_device, &allocInfo, nullptr, &_stagingMemory);
            _vk.vkBindBufferMemory(_device, _stagingBuffer, _stagingMemory, 0);
            _vk.vkMapMemory(_device, _stagingMemory, 0, _stagingBufferSize, 0, &_stagingMappedPtr);
        }

        if (_gameImage == VK_NULL_HANDLE || _gameImageWidth != width || _gameImageHeight != height)
        {
            if (_gameImage != VK_NULL_HANDLE)
            {
                _vk.vkDestroyImage(_device, _gameImage, nullptr);
                _gameImage = VK_NULL_HANDLE;
            }
            if (_gameImageMemory != VK_NULL_HANDLE)
            {
                _vk.vkFreeMemory(_device, _gameImageMemory, nullptr);
                _gameImageMemory = VK_NULL_HANDLE;
            }

            _gameImageWidth = width;
            _gameImageHeight = height;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = _gameImageWidth;
            imageInfo.extent.height = _gameImageHeight;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = _swapchainFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            _vk.vkCreateImage(_device, &imageInfo, nullptr, &_gameImage);

            VkMemoryRequirements memReq{};
            _vk.vkGetImageMemoryRequirements(_device, _gameImage, &memReq);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            _vk.vkAllocateMemory(_device, &allocInfo, nullptr, &_gameImageMemory);
            _vk.vkBindImageMemory(_device, _gameImage, _gameImageMemory, 0);
        }
    }

    void Display()
    {
        if (_device == VK_NULL_HANDLE || _swapchain == VK_NULL_HANDLE || _stagingMappedPtr == nullptr || _width == 0 || _height == 0)
        {
            return;
        }

        _vk.vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX);

        uint32_t imageIndex = 0;
        VkResult acquireRes = _vk.vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (acquireRes == VK_ERROR_OUT_OF_DATE_KHR || acquireRes == VK_SUBOPTIMAL_KHR)
        {
            int32_t dw = 0, dh = 0;
            GetWindowDrawableSize(&dw, &dh);
            if (dw > 0 && dh > 0)
            {
                CreateSwapChain(static_cast<uint32_t>(dw), static_cast<uint32_t>(dh));
            }
            return;
        }

        _vk.vkResetFences(_device, 1, &_inFlightFence);

        // Convert 8-bit palette-indexed bits into 32-bit mapped staging buffer
        uint32_t* dst = static_cast<uint32_t*>(_stagingMappedPtr);
        const PaletteIndex* src = _bits;
        int32_t count = _width * _height;
        int32_t blocks = count / 4;
        int32_t remainder = count % 4;
        while (blocks-- > 0)
        {
            dst[0] = _paletteHWMapped[EnumValue(src[0])];
            dst[1] = _paletteHWMapped[EnumValue(src[1])];
            dst[2] = _paletteHWMapped[EnumValue(src[2])];
            dst[3] = _paletteHWMapped[EnumValue(src[3])];
            src += 4;
            dst += 4;
        }
        while (remainder-- > 0)
        {
            *dst++ = _paletteHWMapped[EnumValue(*src++)];
        }

        // Record command buffer for transfer, blit/scale and presentation
        _vk.vkResetCommandBuffer(_commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        _vk.vkBeginCommandBuffer(_commandBuffer, &beginInfo);

        // Barrier: game image UNDEFINED -> TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _gameImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        _vk.vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Copy staging buffer to game image
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = _width;
        region.bufferImageHeight = _height;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { _width, _height, 1 };

        _vk.vkCmdCopyBufferToImage(_commandBuffer, _stagingBuffer, _gameImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Barrier: game image TRANSFER_DST_OPTIMAL -> TRANSFER_SRC_OPTIMAL
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        _vk.vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Barrier: swapchain image UNDEFINED -> TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier swapchainBarrier{};
        swapchainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.image = _swapchainImages[imageIndex];
        swapchainBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchainBarrier.subresourceRange.baseMipLevel = 0;
        swapchainBarrier.subresourceRange.levelCount = 1;
        swapchainBarrier.subresourceRange.baseArrayLayer = 0;
        swapchainBarrier.subresourceRange.layerCount = 1;
        swapchainBarrier.srcAccessMask = 0;
        swapchainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        _vk.vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapchainBarrier);

        // Blit and scale game image into swapchain image
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { static_cast<int32_t>(_width), static_cast<int32_t>(_height), 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;

        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { static_cast<int32_t>(_swapchainExtent.width), static_cast<int32_t>(_swapchainExtent.height), 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = 0;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        if (!_supportsBlit && _width == _swapchainExtent.width && _height == _swapchainExtent.height && _vk.vkCmdCopyImage != nullptr)
        {
            VkImageCopy copyRegion{};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.extent = { _width, _height, 1 };
            _vk.vkCmdCopyImage(_commandBuffer, _gameImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        }
        else
        {
            VkFilter filter = (Config::Get().general.scaleQuality == ScaleQuality::linear) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
            _vk.vkCmdBlitImage(_commandBuffer, _gameImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);
        }

        // Barrier: swapchain image TRANSFER_DST_OPTIMAL -> PRESENT_SRC_KHR
        swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        swapchainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        swapchainBarrier.dstAccessMask = 0;

        _vk.vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapchainBarrier);

        _vk.vkEndCommandBuffer(_commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffer;

        VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        _vk.vkQueueSubmit(_queue, 1, &submitInfo, _inFlightFence);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &_swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentRes = _vk.vkQueuePresentKHR(_queue, &presentInfo);
        if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR)
        {
            int32_t dw = 0, dh = 0;
            GetWindowDrawableSize(&dw, &dh);
            if (dw > 0 && dh > 0)
            {
                CreateSwapChain(static_cast<uint32_t>(dw), static_cast<uint32_t>(dh));
            }
        }
    }

    void CleanupVulkan()
    {
        if (_device != VK_NULL_HANDLE)
        {
            _vk.vkDeviceWaitIdle(_device);

            if (_stagingMappedPtr != nullptr)
            {
                _vk.vkUnmapMemory(_device, _stagingMemory);
                _stagingMappedPtr = nullptr;
            }
            if (_stagingBuffer != VK_NULL_HANDLE)
            {
                _vk.vkDestroyBuffer(_device, _stagingBuffer, nullptr);
                _stagingBuffer = VK_NULL_HANDLE;
            }
            if (_stagingMemory != VK_NULL_HANDLE)
            {
                _vk.vkFreeMemory(_device, _stagingMemory, nullptr);
                _stagingMemory = VK_NULL_HANDLE;
            }

            if (_gameImage != VK_NULL_HANDLE)
            {
                _vk.vkDestroyImage(_device, _gameImage, nullptr);
                _gameImage = VK_NULL_HANDLE;
            }
            if (_gameImageMemory != VK_NULL_HANDLE)
            {
                _vk.vkFreeMemory(_device, _gameImageMemory, nullptr);
                _gameImageMemory = VK_NULL_HANDLE;
            }

            if (_imageAvailableSemaphore != VK_NULL_HANDLE)
            {
                _vk.vkDestroySemaphore(_device, _imageAvailableSemaphore, nullptr);
                _imageAvailableSemaphore = VK_NULL_HANDLE;
            }
            if (_renderFinishedSemaphore != VK_NULL_HANDLE)
            {
                _vk.vkDestroySemaphore(_device, _renderFinishedSemaphore, nullptr);
                _renderFinishedSemaphore = VK_NULL_HANDLE;
            }
            if (_inFlightFence != VK_NULL_HANDLE)
            {
                _vk.vkDestroyFence(_device, _inFlightFence, nullptr);
                _inFlightFence = VK_NULL_HANDLE;
            }

            if (_commandPool != VK_NULL_HANDLE)
            {
                _vk.vkDestroyCommandPool(_device, _commandPool, nullptr);
                _commandPool = VK_NULL_HANDLE;
            }

            if (_swapchain != VK_NULL_HANDLE)
            {
                _vk.vkDestroySwapchainKHR(_device, _swapchain, nullptr);
                _swapchain = VK_NULL_HANDLE;
            }

            _vk.vkDestroyDevice(_device, nullptr);
            _device = VK_NULL_HANDLE;
        }

        if (_surface != VK_NULL_HANDLE && _instance != VK_NULL_HANDLE)
        {
            _vk.vkDestroySurfaceKHR(_instance, _surface, nullptr);
            _surface = VK_NULL_HANDLE;
        }

        if (_instance != VK_NULL_HANDLE)
        {
            _vk.vkDestroyInstance(_instance, nullptr);
            _instance = VK_NULL_HANDLE;
        }

        if (_vulkanLibHandle != nullptr)
        {
#if defined(_WIN32)
            FreeLibrary(static_cast<HMODULE>(_vulkanLibHandle));
#else
            dlclose(_vulkanLibHandle);
#endif
            _vulkanLibHandle = nullptr;
        }
    }
};

namespace OpenRCT2::Ui
{
    [[nodiscard]] std::unique_ptr<Drawing::IDrawingEngine> CreateVulkanDrawingEngine(IUiContext& uiContext)
    {
        try
        {
            auto engine = std::make_unique<VulkanDrawingEngine>(uiContext);
            engine->Initialise();
            return engine;
        }
        catch (const std::exception& ex)
        {
            LOG_WARNING("Vulkan initialization failed: %s", ex.what());
            return nullptr;
        }
        catch (...)
        {
            LOG_WARNING("Vulkan initialization failed with unknown exception");
            return nullptr;
        }
    }
} // namespace OpenRCT2::Ui

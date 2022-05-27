/*****************************************************************//**
 * \file   LinkerBuildTest.cpp
 * \brief 
 *         This file was adapted from Microsoft's [documentation](https://docs.microsoft.com/en-us/windows/win32/learnwin32/windows-hello-world-sample) 
 *         on using the Win32 API and some vulkan tutorials
 *         for the purposes of testing linker configuration on Visual Studio on the machine
 *         when building.
 * 
 *         It is not designed to be organized and is only currently in use to test linking and
 *         understanding. It's informal and crude so please don't submit it in the final...
 * 
 * \author Clementine Shamaney, clementine.s@digipen.edu
 * \date   May 2022
 *********************************************************************/

// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <optional>

//Vulkan

//win32 api
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include "VulkanInstance.h"
#include <iostream>
#include "LinkerBuildTest.h"
#include "imgui.h"
#include "imgui_impl_win32.h"



namespace VulkanProject
{
    constexpr bool isDebugCallbackOutput = true;

    // The main window class name.
    static TCHAR szWindowClass[] = _T("DesktopApp");

    // The string that appears in the application's title bar.
    static TCHAR szTitle[] = _T("Vulkan Student Project : Clementine");

    // Stored instance handle for use in Win32 API calls such as FindResource
    HINSTANCE hInst;


    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkInstance gVkInstance;
    VkSurfaceKHR win32Surface;
    VkQueue presentationQueue;
    VkQueue graphicsQueue;

    VkSwapchainKHR swapChain;
    VkPresentModeKHR presentMode;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D swapChainExtent;

    VkFormat swapChainImageFormat;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    uint32_t gExtensionCount = 0;
    std::vector<VkExtensionProperties> gExtensionVector(gExtensionCount);

    struct QueueFamilyIndices {
        int graphicsFamilyIndex = -1;
        int presentationFamilyIndex = -1;

        bool isComplete()
        {
            return (graphicsFamilyIndex >= 0 && presentationFamilyIndex >= 0);
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamilyIndex = i;
            }

            VkBool32 presentSupport = false;
            if (win32Surface != nullptr)
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, win32Surface, &presentSupport);

            if (presentSupport) {
                indices.presentationFamilyIndex = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    //Debugging Boilerplate 
    //vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers#page_Debugging-instance-creation-and-destruction
    namespace Debugging
    {
        std::vector<std::string> debugLog;

        VkDebugUtilsMessengerEXT debugMessenger;

        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr) {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
            else {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        }

        void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debugMessenger, pAllocator);
            }
        }
    

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
        {
            debugLog.emplace_back(pCallbackData->pMessage);
            return VK_FALSE;
        }

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;
        }

        void setupDebugMessenger(VkInstance currInstance) {

            VkDebugUtilsMessengerCreateInfoEXT createInfo;
            populateDebugMessengerCreateInfo(createInfo);

            if (CreateDebugUtilsMessengerEXT(currInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                throw std::runtime_error("failed to set up debug messenger!");
            }
        }

        void PrintDebug()
        {
            //Faster than multiple couts
            std::string outputString;
            for (std::string const& str : VulkanProject::Debugging::debugLog)
            {
                outputString += str + "\n\n";
            }
            std::cout << outputString;
        }
    }

    struct graphicsCard
    {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

    };

    graphicsCard currGraphicsCard;


    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(currGraphicsCard.physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;


        if (vkCreateCommandPool(currGraphicsCard.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(currGraphicsCard.logicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
    }

    // Use validation layers if this is a debug build
    std::vector<const char*> g_validationLayers;

    void queryExtensions(uint32_t& extensionCountRef, std::vector<VkExtensionProperties>& extensionVectorRef, bool showNames = true)
    {
        //gotta get the count first before u can put the stuff in that's how this works
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCountRef, nullptr);
        extensionVectorRef.resize(extensionCountRef);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCountRef, extensionVectorRef.data());

        if (!showNames)
            return;

        std::cout << '\t' << "avaliable extensions: " << '\n';
        for (VkExtensionProperties const& e : extensionVectorRef)
            std::cout << '\t' << e.extensionName << '\n';
    }

    void createVulkanInstances(VkInstance& instance)
    {
        std::cout << "createVulkanInstances()\n";

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Linker Test";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Orange Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = 0;

        uint32_t extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensionVector(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionVector.data());

        createInfo.enabledExtensionCount = extensionCount;
        std::vector<const char*> extensionNames(gExtensionCount);
        for (auto const& extension : extensionVector)
            extensionNames.push_back(extension.extensionName);

        createInfo.ppEnabledExtensionNames = extensionNames.data();


        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed: Vulkan Creation Instance"); //Possible test case here in the future?
        }

    }

    //Adapted from: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
    void getGraphicsCard(VkInstance& instance, graphicsCard& graphicsCardStruct)
    {
        //Go through each device to find a dedicated graphics card
        std::cout << "getGraphicsCard()\n";
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("no devices with vulkan support");
        }

        std::vector<VkPhysicalDevice> deviceVector(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, deviceVector.data());


        //Get the dedicated GPU or just any other graphics capable device
        for (VkPhysicalDevice const& device : deviceVector)
        {
            //Get the features of the device
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            //Only allow devices that have queue families

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());


            bool cardSupportsGraphics = false;
            uint32_t queueFamilyIndexGraphics = 0;
            for (const auto& queueFamily : queueFamilies) {

                //Increment until we reach a graphics family.
                ++queueFamilyIndexGraphics;

                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    cardSupportsGraphics = true;
                    
                    break;
                }
            }


            graphicsCardStruct.deviceProperties = deviceProperties;
            graphicsCardStruct.deviceFeatures = deviceFeatures;
            graphicsCardStruct.physicalDevice = device;

            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                //Shows graphics card supportDetails too
                return;
            }
        }

        if (graphicsCardStruct.physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("no graphics device found");
        }

    }

    void makeGraphicsLogicalDevice()
    {
        std::cout << "makeGraphicsLogicalDevice()\n";
        VkDeviceQueueCreateInfo queueCreateInfo{};

        //Gets the queues for the card again for presentation now that the surface is created
        QueueFamilyIndices indices = findQueueFamilies(currGraphicsCard.physicalDevice);

        float graphicsQueuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateGraphics{};
        queueCreateGraphics.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateGraphics.queueFamilyIndex = indices.graphicsFamilyIndex;
        queueCreateGraphics.queueCount = 1;
        queueCreateGraphics.pQueuePriorities = &graphicsQueuePriority;

        float presentQueuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreatePresentation{};
        queueCreatePresentation.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreatePresentation.queueFamilyIndex = indices.presentationFamilyIndex;
        queueCreatePresentation.queueCount = 1;
        queueCreatePresentation.pQueuePriorities = &presentQueuePriority;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfoVector;
        queueCreateInfoVector.push_back(queueCreateGraphics);
        queueCreateInfoVector.push_back(queueCreatePresentation);

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = queueCreateInfoVector.data();

        createInfo.pEnabledFeatures = &currGraphicsCard.deviceFeatures;

        createInfo.enabledLayerCount = g_validationLayers.size();
        createInfo.ppEnabledLayerNames = g_validationLayers.data();

        //Get extensions for graphics card

        uint32_t extensionCountGraphics = 0;
        vkEnumerateDeviceExtensionProperties(currGraphicsCard.physicalDevice, nullptr, &extensionCountGraphics, nullptr);
        std::vector<VkExtensionProperties> extensionGraphicsVector(extensionCountGraphics);

        VkResult result;
        result = vkEnumerateDeviceExtensionProperties(currGraphicsCard.physicalDevice, nullptr, &extensionCountGraphics, extensionGraphicsVector.data());

        //createInfo.enabledExtensionCount = extensionCountGraphics;

        std::vector<const char*> extensionNames;
        for (auto const& p : extensionGraphicsVector)
        {
            extensionNames.push_back(p.extensionName);
        }

        createInfo.enabledExtensionCount = extensionNames.size();
        createInfo.ppEnabledExtensionNames = extensionNames.data();

        result = vkCreateDevice(currGraphicsCard.physicalDevice, &createInfo, nullptr, &currGraphicsCard.logicalDevice);
        if (result != VK_SUCCESS) {
            std::string errorMsg = "failed to create logical device for: ";
            errorMsg += currGraphicsCard.deviceProperties.deviceName;
            throw std::runtime_error(errorMsg);
        }

        vkGetDeviceQueue(currGraphicsCard.logicalDevice, indices.graphicsFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(currGraphicsCard.logicalDevice, indices.presentationFamilyIndex, 0, &presentationQueue);
    }


    //Prototype function for setting up a vulka instance with an example surface view 
    int setupPrototype()
    {
        g_validationLayers.push_back("VK_LAYER_KHRONOS_validation");

        //Attempt to create an instance

        createVulkanInstances(gVkInstance);
        Debugging::setupDebugMessenger(gVkInstance);

        getGraphicsCard(gVkInstance, currGraphicsCard);

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(currGraphicsCard.physicalDevice, &deviceProperties);
        std::cout << "Graphics Card Chosen: " << deviceProperties.deviceName << "\n";

        makeGraphicsLogicalDevice();

        createCommandPool();

        createCommandBuffer();

        return 0;
    }



    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, 
        VkFormat format, VkColorSpaceKHR colorSpace)
        {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == format && availableFormat.colorSpace == colorSpace) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR presentMode) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == presentMode) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, win32Surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, win32Surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, win32Surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, win32Surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, win32Surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    //To Do: Add the error checking for the VK Result stuff here
    int createSwapChain()
    {
        std::cout << "createSwapChain()\n";
        //Adapted from: https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain
        //get the swap chain supportDetails
       
        SwapChainSupportDetails supportDetails = querySwapChainSupport(currGraphicsCard.physicalDevice);

        surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats, VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
        presentMode = chooseSwapPresentMode(supportDetails.presentModes, VK_PRESENT_MODE_FIFO_KHR);

        supportDetails.capabilities.currentExtent.height = static_cast<std::uint32_t>(windowHeight);
        supportDetails.capabilities.currentExtent.width = static_cast<std::uint32_t>(windowWidth);


        swapChainExtent = supportDetails.capabilities.currentExtent;

        VkSurfaceCapabilitiesKHR SurfaceCapabilities{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(currGraphicsCard.physicalDevice, win32Surface, &SurfaceCapabilities);

        swapChainExtent.width  = SurfaceCapabilities.currentExtent.width;
        swapChainExtent.height = SurfaceCapabilities.currentExtent.height;


        uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
        if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount) {
            imageCount = supportDetails.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = win32Surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapChainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //Assume present family is same as graphics family because we doing QUICK PROTOTYPING
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;


        createInfo.preTransform = supportDetails.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(currGraphicsCard.logicalDevice, &createInfo, nullptr, &swapChain);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("cant create swap chain");
        }
        std::cout << "swap chain created for presentation\n";

        vkGetSwapchainImagesKHR(currGraphicsCard.logicalDevice, swapChain, &imageCount, nullptr);

        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(currGraphicsCard.logicalDevice, swapChain, &imageCount, swapChainImages.data());

        return 1;
    }

    void createImage()
    {


    }

    void createImageViews() {
        std::cout << "createImageViews()\n";
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;


            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(currGraphicsCard.logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("createImageViews() failed.");
            }
        }
    }

    void cleanupSwapChain() {
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(currGraphicsCard.logicalDevice, swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(currGraphicsCard.logicalDevice, swapChain, nullptr);
    }



    struct WinMainData
    {
        HINSTANCE hInstance;
        WNDCLASSEX wcex;
        HWND hWnd;

        MSG msg;
    };

    WinMainData currWinMainData;

    //Pass in the win32 api window
    int setupSurface(HWND hwnd, HINSTANCE hInstance)
    {
        VkWin32SurfaceCreateInfoKHR createSurfaceInfo{};
        createSurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createSurfaceInfo.hwnd = hwnd;
        createSurfaceInfo.hinstance = hInstance;

        VkResult result;
        result = vkCreateWin32SurfaceKHR(gVkInstance, &createSurfaceInfo, nullptr, &win32Surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("window surface creation failed");
        }
        else
        {
            std::cout << "Created win32api window surface\n";
        }

        QueueFamilyIndices queueIndices = findQueueFamilies(currGraphicsCard.physicalDevice);

        //Also get the presentation queue here (could prob move this honestly)
        vkGetDeviceQueue(currGraphicsCard.logicalDevice, queueIndices.presentationFamilyIndex, 0, &presentationQueue);

        std::cout << "presentation queue created\n";
    }


    //Finally some code I am not lifting from the tutorial. Test if I can draw straight to the surface without a renderpass
    void presentFrameSimple()
    {
        //Acquire an image that can be used
        uint32_t imageIndex;
        vkAcquireNextImageKHR(currGraphicsCard.logicalDevice, swapChain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

        VkPresentInfoKHR presentInfo;
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        //Implement semaphores later
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.pWaitSemaphores = nullptr;
        presentInfo.pResults = nullptr;

        //https://gist.github.com/TheServer201/26c280d0779423dc714da4a299636ff7

        //Create a clear buffer


        

        //Match the ones we use in imageview
        VkImageSubresourceRange subresourceRange;

        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        VkClearColorValue ClearColorValue = { 0.0, 0.0, 0.86, 0.0 };

        VkCommandBufferBeginInfo CommandBufferBeginInfo;
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.pNext = NULL;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        CommandBufferBeginInfo.pInheritanceInfo = NULL;

        VkResult result;
        result = vkBeginCommandBuffer(commandBuffer, &CommandBufferBeginInfo);
        for (size_t i = 0; i < swapChainImages.size(); ++i)
        {
            vkCmdClearColorImage(commandBuffer, swapChainImages[i], VK_IMAGE_LAYOUT_GENERAL, &ClearColorValue, 1, &subresourceRange);
           
        }
        result = vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 0;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }


        result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }

    bool isQuitting = false;

     //  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
     //
     //  PURPOSE:  Processes messages for the main window.
     //
     //  WM_PAINT    - Paint the main window
     //  WM_DESTROY  - post a quit message and return
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        switch (message)
        {
        case WM_PAINT:
            ValidateRect(hWnd, NULL);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            isQuitting = true;
            //cleanup(); //clear vulkan instances and stuff
            break;
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }


    int WINAPI WinMain(
        _In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR     lpCmdLine,
        _In_ int       nCmdShow
    )
    {
        WNDCLASSEX wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = szWindowClass;
        wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

        if (!RegisterClassEx(&wcex))
        {
            MessageBox(NULL,
                _T("Call to RegisterClassEx failed!"),
                _T("Windows Desktop Guided Tour"),
                NULL);

            return 1;
        }

        currWinMainData.hInstance = hInstance;

        // The parameters to CreateWindowEx explained:
        // WS_EX_OVERLAPPEDWINDOW : An optional extended window style.
        // szWindowClass: the name of the application
        // szTitle: the text that appears in the title bar
        // WS_OVERLAPPEDWINDOW: the type of window to create
        // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
        // NULL: the parent of this window
        // NULL: this application does not have a menu bar
        // hInstance: the first parameter from WinMain
        // NULL: not used in this application
        currWinMainData.hWnd = CreateWindowEx(
            WS_EX_OVERLAPPEDWINDOW,
            szWindowClass,
            szTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            windowWidth, windowHeight,
            NULL,
            NULL,
            hInstance,
            NULL
        );

        if (!currWinMainData.hWnd)
        {
            MessageBox(NULL,
                _T("Call to CreateWindow failed!"),
                _T("Windows Desktop Guided Tour"),
                NULL);

            return 1;
        }



        VulkanProject::setupSurface(currWinMainData.hWnd, currWinMainData.hInstance);
    }

 
    int UpdateWinMain()
    {
        // The parameters to ShowWindow explained:
        // hWnd: the value returned from CreateWindow
        // nCmdShow: the fourth parameter from WinMain
        ShowWindow(currWinMainData.hWnd, SW_SHOWNORMAL);
        UpdateWindow(currWinMainData.hWnd);
        createSwapChain();
        createImageViews();
      
        MSG msg;

        //Render loop
        while (!isQuitting)
        {
            static int num = 0;
            ++num;
            std::cout << "update: " << num << "\n";
            presentFrameSimple();

            if (PeekMessage(&msg, currWinMainData.hWnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }





        return (int)msg.wParam;
    }


    void cleanup()
    {
        std::cout << "cleanup()\n";

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(currGraphicsCard.logicalDevice, imageView, nullptr);
        }

        vkDestroyDevice(currGraphicsCard.logicalDevice, nullptr);
        vkDestroyInstance(gVkInstance, nullptr);
    }
}






int main()
{
    //Can hide the console on demand
    //::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    std::cout << "Vulkan Student Project.";


    VulkanProject::setupPrototype();

    VulkanProject::WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);


    if (VulkanProject::isDebugCallbackOutput)
    {
        VulkanProject::Debugging::PrintDebug();
    }

    
    VulkanProject::UpdateWinMain();

    VulkanProject::cleanup();
}






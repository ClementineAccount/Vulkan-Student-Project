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
#include <fstream>
#include <optional>


#include <unordered_map>

//Vulkan

//win32 api

#define VK_USE_PLATFORM_WIN32_KHR


#include <vulkan/vulkan.hpp>
#include "VulkanInstance.h"
#include <iostream>
#include "LinkerBuildTest.h"
#include "imgui.h"
#include "imgui_impl_win32.h"

constexpr bool isTesting = true;
constexpr bool isDebugCallbackOutput = false;





namespace VulkanProject
{

    const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


    // The main window class name.
    static TCHAR szWindowClass[] = _T("DesktopApp");

    // The string that appears in the application's title bar.
    static TCHAR szTitle[] = _T("Vulkan Student Project : Clementine");

    // Stored instance handle for use in Win32 API calls such as FindResource
    HINSTANCE hInst;

    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;

    VkClearColorValue ClearColorValue = { 1.0, 0.0, 0.0, 0.0 };
    VkClearValue triangleBackground = { 0.0f, 0.0f, 1.0f, 1.0f };

    //---To Do: Move this stuff to a proper global container pattern of sorts---

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

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    //https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    //---To Do: Move this stuff to a proper global container pattern of sorts---

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    std::vector<VkFramebuffer> swapChainFramebuffers;

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


    // Use validation layers
    std::vector<const char*> g_validationLayers;


    //Shader stuff

    std::string shaderFolderPath = "Shaders/";
    std::string vertShaderName = "vert.spv";
    std::string fragShaderName = "frag.spv";

    std::unordered_map<std::string, VkShaderModule> shaderModuleMap;

    std::vector<char> readShaderFile(const std::string& shaderFilePath)
    {
        std::ifstream file(shaderFilePath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open" + shaderFilePath);
        }

        //https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules

        //Allocate a buffer for the fileSize based off the position read by ifstream

        //tellg is at the last position of the binary stream and hence can be used to allocate the buffer size
        size_t fileSize = (size_t)file.tellg(); 
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }




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

    VkDebugUtilsMessengerEXT debugMessenger;

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(gVkInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }



    //std::vector<const char*> getRequiredExtensions() {
    //    uint32_t glfwExtensionCount = 0;
    //    const char** glfwExtensions;
    //    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    //    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    //    if (enableValidationLayers) {
    //        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    //    }

    //    return extensions;
    //}

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : g_validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }




    //https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
    //https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkCreateShaderModule.html
    VkShaderModule createShaderModule(const std::vector<char>& shaderBuffer, const std::string shaderName) {

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderBuffer.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBuffer.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(currGraphicsCard.logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module for" + shaderName);
        }
        else
        {
            std::cout << "shader created for: " << shaderName << std::endl;
        }

        return shaderModule;
    }


    void createShader(std::string const& shaderName)
    {
        //Read the shaders from the filePath
        std::string shaderFilePath = shaderFolderPath;
        shaderFilePath += shaderName;
        VkShaderModule shaderModule = createShaderModule(readShaderFile(shaderFilePath), shaderName);
        shaderModuleMap.insert({ shaderName, shaderModule });
    }


    //void createCommandPool()
    //{
    //    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(currGraphicsCard.physicalDevice);

    //    VkCommandPoolCreateInfo poolInfo{};
    //    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;


    //    if (vkCreateCommandPool(currGraphicsCard.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    //        throw std::runtime_error("failed to create command pool!");
    //    }
    //}

    //void createCommandBuffer()
    //{
    //    VkCommandBufferAllocateInfo allocInfo{};
    //    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //    allocInfo.commandPool = commandPool;
    //    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //    allocInfo.commandBufferCount = 1;

    //    if (vkAllocateCommandBuffers(currGraphicsCard.logicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
    //        throw std::runtime_error("failed to allocate command buffers!");
    //    }
    //}

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
    }


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

        g_validationLayers.push_back("VK_LAYER_KHRONOS_validation");

        if (!checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

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


        //auto extensions = getRequiredExtensions();
        //createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        //createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
            createInfo.ppEnabledLayerNames = g_validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }


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


        //Attempt to create an instance

        createVulkanInstances(gVkInstance);
        Debugging::setupDebugMessenger(gVkInstance);

        getGraphicsCard(gVkInstance, currGraphicsCard);

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(currGraphicsCard.physicalDevice, &deviceProperties);
        std::cout << "Graphics Card Chosen: " << deviceProperties.deviceName << "\n";

        makeGraphicsLogicalDevice();

        //createCommandPool();
        //createCommandBuffer();

        checkValidationLayerSupport();

        return 0;
    }


    //https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
    void setupRenderPass()
    {

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        //Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, 
        //however the layout of the pixels in memory can change based on what you're trying to do with an image. --> from the tutorial

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        //Subpass
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(currGraphicsCard.logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

    }
    
    void setupGraphicsPipeline()
    {
        createShader(vertShaderName);
        createShader(fragShaderName);

        //Shader staging 

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = shaderModuleMap.at(vertShaderName);
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = shaderModuleMap.at(fragShaderName);
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        //https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

        //Vertex Inputs

        //No loading because the tutorial doesn't read data in just yet

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional


        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;


        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(currGraphicsCard.logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(currGraphicsCard.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }


    }

    void cleanupGraphicsPipeline()
    {
        for (auto& mod : shaderModuleMap)
        {
            vkDestroyShaderModule(currGraphicsCard.logicalDevice, mod.second, nullptr);
        }
    }

    void setupFrameBuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(currGraphicsCard.logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer.");
            }
        }
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
        swapChainImageFormat = surfaceFormat.format;
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

    void createCommandBuffer() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(currGraphicsCard.logicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }



    void createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(currGraphicsCard.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(currGraphicsCard.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(currGraphicsCard.logicalDevice, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }

    }

    void recordTriangleCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColorReal = triangleBackground;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColorReal;


        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record triangle comamnd buffer!");
        }
    }




    void drawTriangle()
    {
        vkWaitForFences(currGraphicsCard.logicalDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(currGraphicsCard.logicalDevice, 1, &inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(currGraphicsCard.logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
        recordTriangleCommandBuffer(commandBuffer, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(graphicsQueue, &presentInfo);

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

      
        MSG msg;

        //Render loop
        while (!isQuitting)
        {
            //static int num = 0;
            //++num;
            //std::cout << "update: " << num << "\n";
            //presentFrameSimple();

            
            drawTriangle();

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

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(currGraphicsCard.logicalDevice, framebuffer, nullptr);
        }

        vkDestroyCommandPool(currGraphicsCard.logicalDevice, commandPool, nullptr);

        vkDestroyPipelineLayout(currGraphicsCard.logicalDevice, pipelineLayout, nullptr);
        vkDestroyRenderPass(currGraphicsCard.logicalDevice, renderPass, nullptr);

        vkDestroyDevice(currGraphicsCard.logicalDevice, nullptr);
        vkDestroyInstance(gVkInstance, nullptr);

    }
}


//Simple testing for specific functions 
namespace Testing
{
    void testReadFile()
    {
        //using namespace VulkanProject;

        ////Input by hand 
        //size_t expectedByteSizeVert = 572;
        //size_t expectedByteSizeFrag = 572;

        //assert(readShaderFile("Shaders/triFrag.spv").size() == expectedByteSizeFrag);
        //assert(readShaderFile("Shaders/triVert.spv").size() == expectedByteSizeVert);

        //std::cout << "\n testReadFile() completed \n" << std::endl;
    }
}


int main()
{
    //Can hide the console on demand
    //::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    std::cout << "Vulkan Student Project.";

    if (isTesting)
    {
        Testing::testReadFile();
    }

    VulkanProject::setupPrototype();

    VulkanProject::setupDebugMessenger();

    VulkanProject::WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);


    if (isDebugCallbackOutput)
    {
        VulkanProject::Debugging::PrintDebug();
    }



    VulkanProject::createSwapChain();
    VulkanProject::createImageViews();
    VulkanProject::setupRenderPass();
    VulkanProject::setupGraphicsPipeline();
    VulkanProject::setupFrameBuffers();
    VulkanProject::createCommandPool();
    VulkanProject::createCommandBuffer();
    VulkanProject::createSyncObjects();

    

    VulkanProject::UpdateWinMain();

    VulkanProject::cleanup();
}






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

//Vulkan
#include <vulkan/vulkan.hpp>
#include "VulkanInstance.h"


#include <iostream>

#include "LinkerBuildTest.h"

#include "imgui.h"
#include "imgui_impl_win32.h"

// Global variables

//Global instance for the application for testing purposes
VkInstance gVkInstance;
uint32_t gExtensionCount = 0;
std::vector<VkExtensionProperties> gExtensionVector(gExtensionCount);

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Windows Desktop Guided Tour Application");

// Stored instance handle for use in Win32 API calls such as FindResource
HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool endEarly = true;

constexpr int windowWidth = 800;
constexpr int windowHeight = 600;

struct graphicsCard
{
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    //Which index out of the queue do graphics queue families live in
    uint32_t queueFamilyIndexGraphics;

    std::vector<VkQueueFamilyProperties> queueFamilyVector;
    uint32_t queueFamilyCount;

    VkQueue graphicsQueue;
};

graphicsCard currGraphicsCard;

// Use validation layers if this is a debug build
std::vector<const char*> g_validationLayers;


int vulkanDefault()
{
    return 0;
}

void createVulkanInstances(VkInstance& instance)
{
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

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed: Vulkan Creation Instance"); //Possible test case here in the future?
    }

    vulkanDefault();
}

//Adapted from: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
void getGraphicsCard(VkInstance& instance, graphicsCard& graphicsCardStruct)
{
    //Go through each device to find a dedicated graphics card
    

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

        graphicsCardStruct.queueFamilyIndexGraphics = queueFamilyIndexGraphics;
        graphicsCardStruct.deviceProperties = deviceProperties;
        graphicsCardStruct.deviceFeatures = deviceFeatures;
        graphicsCardStruct.physicalDevice = device;
        graphicsCardStruct.queueFamilyVector = queueFamilies;
        graphicsCardStruct.queueFamilyCount = queueFamilyCount;

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            //Shows graphics card details too
            return;
        }
    }

    if (graphicsCardStruct.physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("no graphics device found");
    }

}

void makeGraphicsLogicalDevice(graphicsCard& graphicsCardStruct)
{
    VkDeviceQueueCreateInfo queueCreateInfo{};

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsCardStruct.queueFamilyIndexGraphics;
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;


    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    
    createInfo.pEnabledFeatures = &graphicsCardStruct.deviceFeatures;
    
    createInfo.enabledExtensionCount = 0;

    createInfo.enabledLayerCount = g_validationLayers.size();
    createInfo.ppEnabledLayerNames = g_validationLayers.data();


    if (vkCreateDevice(graphicsCardStruct.physicalDevice, &createInfo, nullptr, &graphicsCardStruct.logicalDevice) != VK_SUCCESS) {
        std::string errorMsg = "failed to create logical device for: ";
        errorMsg += graphicsCardStruct.deviceProperties.deviceName;
        throw std::runtime_error(errorMsg);
    }


    //How many queues you want to create from this call
    uint32_t numGraphicsQueues = 0;
    vkGetDeviceQueue(graphicsCardStruct.logicalDevice, graphicsCardStruct.queueFamilyIndexGraphics, numGraphicsQueues, &graphicsCardStruct.graphicsQueue);

}

//Creating a wrapper just to call one function and a loop is good game design
void queryExtensions(uint32_t &extensionCountRef, std::vector<VkExtensionProperties>& extensionVectorRef, bool showNames = true)
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


//Prototype function for setting up a 
int setupPrototype()
{

#if defined(_DEBUG)
    g_validationLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    //Attempt to create an instance
    createVulkanInstances(gVkInstance);
    queryExtensions(gExtensionCount, gExtensionVector);
    getGraphicsCard(gVkInstance, currGraphicsCard);

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(currGraphicsCard.physicalDevice, &deviceProperties);
    std::cout << "Graphics Card Chosen: " << deviceProperties.deviceName << "\n";

    makeGraphicsLogicalDevice(currGraphicsCard);




    return 0;
}

int main()
{
    //Can hide the console on demand
    //::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    std::cout << "Hello Console.";

    setupPrototype();


    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);

}


void cleanup()
{
    vkDestroyInstance(gVkInstance, nullptr);
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

    // Store instance handle in our global variable
    hInst = hInstance;

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
    HWND hWnd = CreateWindowEx(
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

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // Setup Platform/Renderer backends
    //ImGui_ImplWin32_Init(hWnd);



    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Vulkan Linker Test");

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        // Here your application is laid out.
        // For this introduction, we just print out "Hello, Windows desktop!"
        // in the top left corner.
        TextOut(hdc,
            5, 5,
            greeting, _tcslen(greeting));
        // End application-specific layout section.

        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        cleanup(); //clear vulkan instances and stuff
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}
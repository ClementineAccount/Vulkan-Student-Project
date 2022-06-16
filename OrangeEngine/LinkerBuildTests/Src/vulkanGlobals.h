#pragma once



#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <fstream>
#include <optional>

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include "Texture.h"

namespace VulkanProject
{
    struct modelTransforms
    {
        glm::vec3 pos;
        glm::vec3 rotDegrees;
        glm::vec3 scale;
    };

    //For model view transformations
    struct modelObject
    {
        Model model;
        modelTransforms trans;

    };


    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer vertexBuffer2;
    VkDeviceMemory vertexBufferMemory2;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkCommandPool commandPool;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VkInstance gVkInstance;

    VkQueue presentationQueue;
    VkQueue graphicsQueue;

    VkSwapchainKHR swapChain;
    VkPresentModeKHR presentMode;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D swapChainExtent;

    VkPipelineLayout _meshPipelineLayout;

    glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

    glm::vec3 defaultCameraPos(3.0f, 3.0f, 3.0f);
    glm::vec3 defaultCameraTarget(0.0f, 0.0f, 0.0f);

    struct Camera
    {
        glm::vec3 pos;
        glm::vec3 target;
    };


    float modelScale = 0.01f;
    float cameraSpeed = 0.01f;
    Camera camera;

    glm::vec3 defaultLightPos = { 0.3f, 3.2f, 0.0f };
    glm::vec3 defaultLightColor = { 1.0f, 1.0f, 1.0f };


    struct Light
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    Light pointLight;


    float deltaTime;


    std::unordered_map<std::string, Texture> textureMap;
    

    //tbh I don't know why I don't use an enum class instead as on principle thats much faster
    namespace TextureNames
    {
        const char carBase[] = "carBase";
        const char carNormal[] = "carNormal";
        const char carRough[] = "carRough";
        const char carAO[] = "carAO";
        const char carMetal[] = "carMetal";

        const char skullBase[] = "skullBase";
        const char skullNormal[] = "skullNormal";
        const char skullRough[] = "skullRough";
        const char skullAO[] = "skullAO";
        const char skullMetal[] = "skullMetal";

    }
}

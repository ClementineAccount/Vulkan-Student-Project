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

    std::unordered_map<std::string, Texture> textureMap;
    

    //tbh I don't know why I don't use an enum class instead as on principle thats much faster
    namespace TextureNames
    {
        const char carBase[] = "carBase";
        const char carNormal[] = "carNormal";
        const char carRough[] = "carRough";
        const char carAO[] = "carAO";
        const char carMetal[] = "carMetal";
    }
}

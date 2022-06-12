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


}

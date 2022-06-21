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
#include <climits>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "tinyddsloader.h"

#include <iostream>

#include "Mesh.h"
#include "vulkanGlobals.h"

//Vulkan

//win32 api



//#define DEFAULT_CUBE



constexpr bool isSkullModel = true;

constexpr bool isTesting = true;
constexpr bool isDebugCallbackOutput = false;

namespace VulkanProject
{

    struct graphicsCard
    {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

    };

    graphicsCard currGraphicsCard;


    Model currModel;
    
    VkSurfaceKHR win32Surface;
    const int MAX_FRAMES_IN_FLIGHT = 2;

    const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // The main window class name.
    static TCHAR szWindowClass[] = _T("DesktopApp");

    // The string that appears in the application's title bar.
    static TCHAR szTitle[] = _T("Vulkan Student Project : Clementine");

    // Stored instance handle for use in Win32 API calls such as FindResource
    HINSTANCE hInst;



    uint32_t currentFrame = 0;

    VkClearColorValue ClearColorValue = { 1.0, 0.0, 0.0, 0.0 };
    VkClearValue triangleBackground = { 0.0f, 0.0f, 1.0f, 1.0f };

    //---To Do: Move this stuff to a proper global container pattern of sorts---



    //This section mostly adapted from:
    //https://vulkan-tutorial.com/Texture_mapping/Images

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(currGraphicsCard.physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkCommandBuffer beginCommand() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(currGraphicsCard.logicalDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endCommand(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(currGraphicsCard.logicalDevice, commandPool, 1, &commandBuffer);
    }



    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginCommand();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endCommand(commandBuffer);
    }



    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginCommand();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endCommand(commandBuffer);
    }



    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(currGraphicsCard.logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(currGraphicsCard.logicalDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(currGraphicsCard.logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(currGraphicsCard.logicalDevice, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(currGraphicsCard.logicalDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(currGraphicsCard.logicalDevice, commandPool, 1, &commandBuffer);
    }

    


    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(currGraphicsCard.logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(currGraphicsCard.logicalDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(currGraphicsCard.logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(currGraphicsCard.logicalDevice, image, imageMemory, 0);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(currGraphicsCard.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void createTextureSampler(VkSampler& textureSampler) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(currGraphicsCard.physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(currGraphicsCard.logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }



    void makeTexture(std::string const& filePath, Texture& textureRef)
    {

        tinyddsloader::Result result = textureRef.ddsImage.Load(filePath.c_str());
        if (result != tinyddsloader::Result::Success)
        {
            throw std::runtime_error("failed to load texture at: " + filePath);
        }

        //tinyddsloader::DDSFile::DXGIFormat imageFormat;
        //if (Vertex::textureFormat == VK_FORMAT_R32G32B32A32_SFLOAT)
        //{
        //    imageFormat = tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Float;
        //}

        int height = textureRef.ddsImage.GetHeight();
        int width = textureRef.ddsImage.GetWidth();

        tinyddsloader::DDSFile::DXGIFormat imageFormat = textureRef.ddsImage.GetFormat();
        unsigned int bitsPerPixel = textureRef.ddsImage.GetBitsPerPixel(imageFormat);
        float bytesPerPixel = (float) bitsPerPixel / (float) CHAR_BIT;

        VkDeviceSize imageSize = height * width * bytesPerPixel;

        //To Do: case switch for all possible formats?
        VkFormat textureFormat = VK_FORMAT_BC3_UNORM_BLOCK;
        if (imageFormat == tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm)
        {
            textureFormat = VK_FORMAT_BC3_UNORM_BLOCK;
        }
        else if (imageFormat == tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB)
        {
            textureFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        }
        else if (imageFormat == tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm)
        {
            textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
        }
        else if (imageFormat == tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm)
        {
            textureFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        }
        else if (imageFormat == tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm)
        {
            textureFormat = VK_FORMAT_BC5_UNORM_BLOCK;
        }


        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(currGraphicsCard.logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, textureRef.ddsImage.GetImageData()->m_mem, static_cast<size_t>(imageSize));
        vkUnmapMemory(currGraphicsCard.logicalDevice, stagingBufferMemory);

        createImage(height, width, textureFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureRef.textureImage, textureRef.textureImageMemory);

        transitionImageLayout(textureRef.textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, textureRef.textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        transitionImageLayout(textureRef.textureImage, textureFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(currGraphicsCard.logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(currGraphicsCard.logicalDevice, stagingBufferMemory, nullptr);

        textureRef.textureImageView = createImageView(textureRef.textureImage, textureFormat);

        createTextureSampler(textureRef.textureSampler);
    };


    std::vector<Texture> textureVector;


    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 biTangent;


        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static const VkFormat textureFormat = VK_FORMAT_R32G32_SFLOAT;
        static const VkFormat vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        static const VkFormat colorFormat = VK_FORMAT_R32G32B32_SFLOAT;

        static const VkFormat normalFormat = VK_FORMAT_R32G32B32_SFLOAT;
        static const VkFormat tangentFormat = VK_FORMAT_R32G32B32_SFLOAT;
        static const VkFormat biTangetFormat = VK_FORMAT_R32G32B32_SFLOAT;

        static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vertexFormat;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = colorFormat;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = textureFormat;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = normalFormat;
            attributeDescriptions[3].offset = offsetof(Vertex, normal);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = tangentFormat;
            attributeDescriptions[4].offset = offsetof(Vertex, tangent);

            attributeDescriptions[5].binding = 0;
            attributeDescriptions[5].location = 5;
            attributeDescriptions[5].format = biTangetFormat;
            attributeDescriptions[5].offset = offsetof(Vertex, biTangent);

            return attributeDescriptions;
        }
    };



    //SOA to AOS conversion  helper function
    std::vector<Vertex> VerticesToBuffer(Vertices const& vertices)
    {
        static const glm::vec3 defaultColor = { 1.0f, 0.0f, 0.0f };
        static const glm::vec3 defaultNormal = { 0.0f, 0.0f, 0.0f };
        static const glm::vec2 defaultTexCord = { 0.0f, 0.0f };
        static const glm::vec3 defaultTangent = { 0.0f, 0.0f, 0.0f };
        static const glm::vec3 defaultBiTanget = { 0.0f, 0.0f, 0.0f };

        std::vector<Vertex> vertexList;
        for (size_t i = 0; i < vertices.positions.size(); ++i)
        {
            Vertex vertexToInsert;
            vertexToInsert.pos = vertices.positions[i];
            vertexList.push_back(vertexToInsert);

            if (i < vertices.colors.size())
            {
                vertexList[i].color = vertices.colors[i];
            }
            else
            {
                vertexList[i].color = defaultColor;
            }

            if (i < vertices.textureCords.size())
            {
                vertexList[i].texCoord = vertices.textureCords[i];
            }
            else
            {
                vertexList[i].texCoord = defaultTexCord;
            }

            if (i < vertices.normals.size())
            {
                vertexList[i].normal = vertices.normals[i];
            }
            else
            {
                vertexList[i].normal = defaultNormal;
            }

            if (i < vertices.tangent.size())
            {
                vertexList[i].tangent = vertices.tangent[i];
            }
            else
            {
                vertexList[i].tangent = defaultTangent;
            }

            if (i < vertices.biTangent.size())
            {
                vertexList[i].biTangent = vertices.biTangent[i];
            }
            else
            {
                vertexList[i].biTangent = defaultBiTanget;
            }
        }

        return vertexList;
    }

    void createVertexBuffer(std::vector<Vertex> const& vertexList) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertexList[0]) * vertexList.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(currGraphicsCard.logicalDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(currGraphicsCard.logicalDevice, vertexBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(currGraphicsCard.logicalDevice, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(currGraphicsCard.logicalDevice, vertexBuffer, vertexBufferMemory, 0);


        void* data;
        vkMapMemory(currGraphicsCard.logicalDevice, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertexList.data(), (size_t)bufferInfo.size);
        vkUnmapMemory(currGraphicsCard.logicalDevice, vertexBufferMemory);

    }

    void createIndexBuffer(std::vector<Indices::indicesType> const& indexList) {
        VkDeviceSize bufferSize = sizeof(indexList[0]) * indexList.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(currGraphicsCard.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indexList.data(), (size_t)bufferSize);
        vkUnmapMemory(currGraphicsCard.logicalDevice, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(currGraphicsCard.logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(currGraphicsCard.logicalDevice, stagingBufferMemory, nullptr);
    }

    void createVertexBufferFromVertices(Vertices verticesList, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory)
    {
        std::vector<Vertex> vertexList = VerticesToBuffer(verticesList);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertexList[0]) * vertexList.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(currGraphicsCard.logicalDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(currGraphicsCard.logicalDevice, vertexBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(currGraphicsCard.logicalDevice, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(currGraphicsCard.logicalDevice, vertexBuffer, vertexBufferMemory, 0);

        void* data;
        vkMapMemory(currGraphicsCard.logicalDevice, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertexList.data(), (size_t)bufferInfo.size);
        vkUnmapMemory(currGraphicsCard.logicalDevice, vertexBufferMemory);
    }


    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    const std::vector<Vertex> vertices = {

        //Front Face
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, 
        {{  0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, 
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, 
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},

        // Back face
        {{ -0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  -0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},

        // Top face
        {{  -0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{  -0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
        {{  0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},

        // Bottom Face
        {{ -0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},

        //Right Face
        {{  0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{   0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},

        // Left face
        {{   -0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{   -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  -0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  -0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    };
    std::vector<Vertex> vertex_load_model;
    Mesh meshLoad;
    



    const std::vector<Indices::indicesType> indices = {
        0, 1, 2, 0, 2, 3, //front
        4, 5, 6, 4, 6, 7, //back
        8, 9, 10, 8, 10, 11, //top
        12, 13, 14, 12, 14, 15, //bottom
        16, 17, 18, 16, 18, 19, //right
        20, 21, 22, 20, 22, 23 //left
    };

    struct MeshPushConstants {
        glm::vec4 light_pos;
        glm::vec4 light_color;
        glm::vec4 camera_pos;
        glm::mat4 model_matrix;
        glm::mat4 render_matrix;
    };



    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorSetLayout descriptorSetLayout;

    VkFormat swapChainImageFormat;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

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

    //https://vkguide.dev/docs/chapter-3/push_constants/

    struct cameraPushConstant
    {
        glm::mat4 mvp_matrix;
    };



    // Use validation layers
    std::vector<const char*> g_validationLayers;

    //Shader stuff
    std::string shaderFolderPath = "Shaders/";
    std::string vertShaderName = "vertmvp.spv";
    std::string fragShaderName = "fragmvp.spv";

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

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 5; //To Do: Make a way to have this update without having to hardcode
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding2{};
        samplerLayoutBinding2.binding = 2;
        samplerLayoutBinding2.descriptorCount = 1;
        samplerLayoutBinding2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding2.pImmutableSamplers = nullptr;
        samplerLayoutBinding2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, samplerLayoutBinding2 };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(currGraphicsCard.logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    //https://vulkan-tutorial.com/Depth_buffering

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(currGraphicsCard.physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }




    void createIndexBufferFromList(std::vector<Indices::indicesType> const& indexList, VkBuffer& myIndexBuffer, VkDeviceMemory& myIndexBufferMemory)
    {
        VkDeviceSize bufferSize = sizeof(indexList[0]) * indexList.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(currGraphicsCard.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indexList.data(), (size_t)bufferSize);
        vkUnmapMemory(currGraphicsCard.logicalDevice, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, myIndexBuffer, myIndexBufferMemory);

        copyBuffer(stagingBuffer, myIndexBuffer, bufferSize);

        vkDestroyBuffer(currGraphicsCard.logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(currGraphicsCard.logicalDevice, stagingBufferMemory, nullptr);
    }

    void createBuffersFromModel(Model& currModel)
    {
        for (Mesh& mesh : currModel.meshVector)
        {
            createVertexBufferFromVertices(mesh.meshVertices, mesh.vertexBuffer, mesh.vertexBufferMemory);
            createIndexBufferFromList(mesh.meshIndices.indexVector, mesh.indexBuffer, mesh.indexBufferMemory);
        }
    }

    void updateUniformBuffer(uint32_t currentImage, glm::vec3 translate = glm::vec3(0.0f, 0.0f, 0.0f)) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::translate(glm::mat4(1.0f), translate);
        ubo.model = glm::rotate(ubo.model, deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void* data;
        vkMapMemory(currGraphicsCard.logicalDevice , uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(currGraphicsCard.logicalDevice, uniformBuffersMemory[currentImage]);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(currGraphicsCard.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(currGraphicsCard.logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);


            auto makeImageInfo = [](std::string textureName)
            {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = textureMap[textureName].textureImageView;
                imageInfo.sampler = textureMap[textureName].textureSampler;
                return imageInfo;
            };


            VkDescriptorImageInfo roughImage;
            VkDescriptorImageInfo baseImage;
            VkDescriptorImageInfo normalImage;
            VkDescriptorImageInfo AOImage;
            VkDescriptorImageInfo metalImage;


            if (isSkullModel)
            {
                roughImage = makeImageInfo(TextureNames::skullRough);
                baseImage = makeImageInfo(TextureNames::skullBase);
                normalImage = makeImageInfo(TextureNames::skullNormal);
                AOImage = makeImageInfo(TextureNames::skullAO);
                metalImage = makeImageInfo(TextureNames::skullMetal);
            }
            else
            {
                roughImage = makeImageInfo(TextureNames::carRough);
                baseImage = makeImageInfo(TextureNames::carBase);
                normalImage = makeImageInfo(TextureNames::carNormal);
                AOImage = makeImageInfo(TextureNames::carAO);
                metalImage = makeImageInfo(TextureNames::carMetal);
            }



            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            std::array <VkDescriptorImageInfo, 5> imageInfoArray;
            imageInfoArray[0] = baseImage;
            imageInfoArray[1] = normalImage;
            imageInfoArray[2] = roughImage;
            imageInfoArray[3] = AOImage;
            imageInfoArray[4] = metalImage;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = imageInfoArray.size();
            descriptorWrites[1].pImageInfo = imageInfoArray.data();


            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &baseImage;

            vkUpdateDescriptorSets(currGraphicsCard.logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
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
        appInfo.pApplicationName = "Vulkan Student Project";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 1, 0);
        appInfo.pEngineName = "Vulkan Student Project";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = 0;

        uint32_t extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensionVector(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionVector.data());


        std::vector<const char*> extensionNames(gExtensionCount);

        extensionNames.push_back("VK_KHR_surface");
        extensionNames.push_back("VK_KHR_win32_surface");
        //extensionNames.push_back("VK_KHR_surface_protected_capabilities");


        //for (auto const& extension : extensionVector)
        //    extensionNames.push_back(extension.extensionName);


        createInfo.enabledExtensionCount = extensionNames.size();
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

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfoVector;
        queueCreateInfoVector.push_back(queueCreateGraphics);

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
        extensionNames.push_back("VK_KHR_swapchain");

        //std::string removeExtension = "VK_EXT_buffer_device_address";
        //

        //for (auto const& p : extensionGraphicsVector)
        //{
        //    extensionNames.push_back(p.extensionName);
        //}
        //extensionNames.erase(std::remove(extensionNames.begin(), extensionNames.end(), removeExtension), extensionNames.end());

       //removeExtension = "VK_KHR_get_surface_capabilities2";
       //removeExtension = "VK_KHR_surface_protected_capabilities";

       //extensionNames.erase(std::remove(extensionNames.begin(), extensionNames.end(), removeExtension), extensionNames.end());


        createInfo.enabledExtensionCount = extensionNames.size();
        createInfo.ppEnabledExtensionNames = extensionNames.data();

        result = vkCreateDevice(currGraphicsCard.physicalDevice, &createInfo, nullptr, &currGraphicsCard.logicalDevice);
        if (result != VK_SUCCESS) {
            std::string errorMsg = "failed to create logical device for: ";
            errorMsg += currGraphicsCard.deviceProperties.deviceName;
            throw std::runtime_error(errorMsg);
        }

        vkGetDeviceQueue(currGraphicsCard.logicalDevice, indices.graphicsFamilyIndex, 0, &graphicsQueue);
    }


    //Prototype function for setting up a vulka instance with an example surface view 
    int setupPrototype()
    {


        //Attempt to create an instance

        createVulkanInstances(gVkInstance);
        //Debugging::setupDebugMessenger(gVkInstance);

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
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        //Subpass
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;


        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(currGraphicsCard.logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

    }
    
    void setupGraphicsPipeline()
    {
        createShader(vertShaderName);
        createShader(fragShaderName);

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

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

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
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;



        //https://vkguide.dev/docs/chapter-3/push_constants/

        //setup push constants
        VkPushConstantRange push_constant;
        //this push constant range starts at the beginning
        push_constant.offset = 0;
        //this push constant range takes up the size of a MeshPushConstants struct
        push_constant.size = sizeof(MeshPushConstants);
        //this push constant range is accessible only in the vertex shader
        push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pipelineLayoutInfo.pPushConstantRanges = &push_constant;
        pipelineLayoutInfo.pushConstantRangeCount = 1;

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
        pipelineInfo.pDepthStencilState = &depthStencil;
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
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
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


    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
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
        result = vkBeginCommandBuffer(commandBuffers[0], &CommandBufferBeginInfo);
        for (size_t i = 0; i < swapChainImages.size(); ++i)
        {
            vkCmdClearColorImage(commandBuffers[0], swapChainImages[i], VK_IMAGE_LAYOUT_GENERAL, &ClearColorValue, 1, &subresourceRange);
           
        }
        result = vkEndCommandBuffer(commandBuffers[0]);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 0;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[0];

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

    void createCommandBuffers() {

        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(currGraphicsCard.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void createSyncObjects() {

        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);


        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (vkCreateSemaphore(currGraphicsCard.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(currGraphicsCard.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(currGraphicsCard.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }


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

        VkClearValue clearColorReal = {{ 0.0f, 0.0f, 0.0f, 0.0f }};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColorReal;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);


        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);



        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        glm::mat4 view = glm::mat4(1.f);

        glm::mat4 model;
        glm::mat4 projection;

        model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

        //camPos.x += time * 0.01f;
        //camPos.y += time * 0.01f;
        //camPos.z += time * 0.01f;

        view = glm::lookAt(camera.pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        projection = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 1000.0f);
        projection[1][1] *= -1;

        //calculate final mesh matrix
        glm::mat4 mesh_matrix = projection * view * model;

        MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;

        //upload the matrix to the GPU via push constants
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);


        //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
#ifdef DEFAULT_CUBE
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
#else
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshLoad.meshIndices.indexVector.size()), 1, 0, 0, 0);
#endif


        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record triangle comamnd buffer!");
        }
    }

    void cleanupSwapChain() {
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(currGraphicsCard.logicalDevice, swapChainFramebuffers[i], nullptr);
        }

        vkDestroyPipeline(currGraphicsCard.logicalDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(currGraphicsCard.logicalDevice, pipelineLayout, nullptr);
        vkDestroyRenderPass(currGraphicsCard.logicalDevice, renderPass, nullptr);

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(currGraphicsCard.logicalDevice, swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(currGraphicsCard.logicalDevice, swapChain, nullptr);

        vkDestroyImageView(currGraphicsCard.logicalDevice, depthImageView, nullptr);
        vkDestroyImage(currGraphicsCard.logicalDevice, depthImage, nullptr);
        vkFreeMemory(currGraphicsCard.logicalDevice, depthImageMemory, nullptr);

        for (auto& mod : shaderModuleMap)
            vkDestroyShaderModule(currGraphicsCard.logicalDevice, mod.second, nullptr);
    }

    void recreateSwapChain() {

        RECT rect;
        GetWindowRect(currWinMainData.hWnd, &rect);
        windowWidth = rect.right - rect.left;
        windowHeight = rect.bottom - rect.top;
        while (windowWidth == 0 || windowHeight == 0)
        {
            GetWindowRect(currWinMainData.hWnd, &rect);
            windowWidth = rect.right - rect.left;
            windowHeight = rect.bottom - rect.top;
        }

        vkDeviceWaitIdle(currGraphicsCard.logicalDevice);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        setupRenderPass();
        setupGraphicsPipeline();

        createDepthResources();
        setupFrameBuffers();
    }

    void drawTriangle(glm::vec3 translate = glm::vec3(0.f, 0.0f, 0.0f))
    {
        vkWaitForFences(currGraphicsCard.logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(currGraphicsCard.logicalDevice, 1, &inFlightFences[currentFrame]);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(currGraphicsCard.logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordTriangleCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    }

    void recordModelCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Model const& modelDraw, glm::vec3 translate = glm::vec3(0.f, 0.0f, 0.0f))
    {
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

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        for (Mesh& mesh : currModel.meshVector)
        {
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.vertexBuffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
        }


        //void* data;
        //vkMapMemory(currGraphicsCard.logicalDevice, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        //memcpy(data, &ubo, sizeof(ubo));
        //vkUnmapMemory(currGraphicsCard.logicalDevice, uniformBuffersMemory[currentImage]);


        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        glm::mat4 view = glm::mat4(1.f);

        glm::mat4 modelMat;
        glm::mat4 projection;

        modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f));
        modelMat = glm::rotate(modelMat, deltaTime * glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(modelScale, modelScale, modelScale));





        auto getKeyDown = [](int virtKey)
        {
            return GetKeyState(virtKey) < 0;
        };

        auto getKeyToggle = [](int virtKey)
        {
            return GetKeyState(virtKey & 1) != 0;
        };



        //very simple wasd camera controls

        //A key code 
        static int AKeyCode = 0x41;
        static int DKeyCode = 0x44;
        static int SKeyCode = 0x53;
        static int WKeyCode = 0x57;
        static int QKeyCode = 0x51;
        static int EKeyCode = 0x45;
        static int RKeyCode = 0x52;
        static int FKeyCode = 0x46;

        //Reset
        if (getKeyDown(FKeyCode))
        {
            camera.pos = defaultCameraPos;
            camera.target = defaultCameraTarget;

        }

        if (getKeyDown(RKeyCode) || getKeyDown(VK_RBUTTON))
        {
            //https://learnopengl.com/Getting-started/Camera
            RECT rect;
            GetWindowRect(currWinMainData.hWnd, &rect);
            ClipCursor(&rect);

            int x = 0;
            int y = 0;
            POINT point{ x, y };
            GetCursorPos(&point);
            ScreenToClient(currWinMainData.hWnd, &point);
            x = point.x;
            y = point.y;
            y = windowHeight - y;

            if (camera.firstMouse)
            {
                camera.lastX = x;
                camera.lastY = y;
                camera.firstMouse = false;
            }

            float xoffset = x - camera.lastX;
            float yoffset = y - camera.lastY;

            camera.lastX = x;
            camera.lastY = y;

            xoffset *= camera.sens;
            yoffset *= camera.sens;

            camera.yaw += xoffset;
            camera.pitch += yoffset;

            if (camera.pitch > 89.0f)
                camera.pitch = 89.0f;
            if (camera.pitch < -89.0f)
                camera.pitch = -89.0f;


            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));

            camera.front = glm::normalize(front);
            // also re-calculate the Right and Up vector
            camera.right = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
            camera.up = glm::normalize(glm::cross(camera.right, camera.front));

            camera.target = camera.pos + camera.front;
        }
        else
        {
            camera.firstMouse = true;
        }

        if (getKeyDown(EKeyCode))
        {
            camera.pos -= camera.up * cameraSpeed * deltaTime;
            camera.target -= camera.up * cameraSpeed * deltaTime;
        }

        if (getKeyDown(QKeyCode))
        {
            camera.pos += camera.up * cameraSpeed * deltaTime;
            camera.target += camera.up * cameraSpeed * deltaTime;
        }


        if (getKeyDown(AKeyCode) || getKeyDown(VK_LEFT))
        {
            camera.pos -= camera.right * cameraSpeed * deltaTime;
            camera.target -= camera.right * cameraSpeed * deltaTime;
        }
        if (getKeyDown(DKeyCode) || getKeyDown(VK_RIGHT))
        {
            camera.pos += camera.right * cameraSpeed * deltaTime;
            camera.target += camera.right * cameraSpeed * deltaTime;
        }

        if (getKeyDown(SKeyCode) || getKeyDown(VK_DOWN))
        {
            camera.pos -= camera.front * cameraSpeed * deltaTime;
            camera.target -= camera.front * cameraSpeed * deltaTime;
        }

        if (getKeyDown(WKeyCode) || getKeyDown(VK_UP))
        {
            camera.pos += camera.front * cameraSpeed * deltaTime;
            camera.target += camera.front * cameraSpeed * deltaTime;
        }

        if (getKeyDown(VK_SPACE))
        {
            pointLight.pos = camera.pos;
        }


        //testing light colors

        pointLight.color = glm::vec3(1.0f, 1.0f, 1.0f);

        //red (r key)
        if (getKeyDown(0x52))
        {
            //pointLight.color = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        //green (g key)
        if (getKeyDown(0x47))
        {
            pointLight.color = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        //blue (b key)
        if (getKeyDown(0x42))
        {
            pointLight.color = glm::vec3(0.0f, 0.0f, 1.0f);
        }




        view = glm::lookAt(camera.pos, camera.target, camera.up );
        projection = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 1000.0f);
        projection[1][1] *= -1;

        //calculate final mesh matrix
        glm::mat4 mesh_matrix = projection * view * modelMat;

        MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;
        constants.model_matrix = modelMat;
        constants.light_pos = glm::vec4(pointLight.pos, 0.0f);
        constants.camera_pos = glm::vec4(camera.pos, 0.0f);
        constants.light_color = glm::vec4(pointLight.color, 0.0f);

        //upload the matrix to the GPU via push constants
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
#ifdef DEFAULT_CUBE
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
#else
        //Loop through every mesh and draw the index accordingly

        for (Mesh& mesh : currModel.meshVector)
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.meshIndices.indexVector.size()), 1, 0, 0, 0);
#endif


        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record triangle comamnd buffer!");
        }
    }

    void drawModel(Model const& model)
    {
        vkWaitForFences(currGraphicsCard.logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(currGraphicsCard.logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
        }

        vkResetFences(currGraphicsCard.logicalDevice, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

        recordModelCommandBuffer(commandBuffers[currentFrame], imageIndex, model);

        //recordTriangleCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
        }


        if (result != VK_SUCCESS) {
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

        //vkQueuePresentKHR(graphicsQueue, &presentInfo);

        result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void loadObjects()
    {
        std::string sphereMod = "Models/sphere_modified.obj";
        std::string sphere = "Models/sphere.obj";
        std::string quad = "Models/quad.obj";
        std::string cup = "Models/cup.obj";
        std::string starwars1 = "Models/starwars1.obj";
        std::string cubeFilePath = "Models/cube2.obj";
        std::string skullFilePath = "Models/demon-skull/skull.fbx";
        std::string carFilePath = "Models/vintage-car/car.fbx";
        

        std::string textureCube = "Models/cubeTexture.obj";
        std::string fourSphere = "Models/4Sphere.obj";
        std::string cuteAngel = "Models/lucy_princeton.obj";


        //car 

        if (isSkullModel)
        {
            currModel.loadModel(skullFilePath);

            std::string textureFilePath = "Models/demon-skull/textures/";

            std::string skullBaseColorPath = textureFilePath + "TD_Checker_Base_Color.dds";
            std::string skullNormalPath = textureFilePath + "TD_Checker_Normal_OpenGL.dds";
            std::string skullRoughnessPath = textureFilePath + "TD_Checker_Roughness.dds";
            std::string skullAOPath = textureFilePath + "TD_Checker_Mixed_AO.dds";
            std::string skullMetalPath = textureFilePath + "_Metallic.dds";


            std::unique_ptr<Texture> baseTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> normalTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> roughTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> AOTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> metallicTexture = std::make_unique<Texture>();

            makeTexture(skullBaseColorPath, *baseTexture.get());
            textureMap.insert({ TextureNames::skullBase, *baseTexture.get() });

            makeTexture(skullNormalPath, *normalTexture);
            textureMap.insert({ TextureNames::skullNormal, *normalTexture });

            makeTexture(skullRoughnessPath, *roughTexture);
            textureMap.insert({ TextureNames::skullRough, *roughTexture });

            makeTexture(skullAOPath, *AOTexture);
            textureMap.insert({ TextureNames::skullAO, *AOTexture });

            makeTexture(skullMetalPath, *metallicTexture);
            textureMap.insert({ TextureNames::skullMetal, *metallicTexture });
        }
        else //car model
        {
            currModel.loadModel(carFilePath);

            std::string textureFilePath = "Models/vintage-car/textures/";

            std::string carBaseColorPath = textureFilePath + "_Base_Color.dds";
            std::string carNormalPath = textureFilePath + "_Normal_DirectX.dds";
            std::string carRoughnessPath = textureFilePath + "_Roughness.dds";
            std::string carAOPath = textureFilePath + "_Mixed_AO.dds";
            std::string carMetalPath = textureFilePath + "_Metallic.dds";


            std::unique_ptr<Texture> baseTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> normalTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> roughTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> AOTexture = std::make_unique<Texture>();
            std::unique_ptr<Texture> metallicTexture = std::make_unique<Texture>();

            makeTexture(carBaseColorPath, *baseTexture.get());
            textureMap.insert({ TextureNames::carBase, *baseTexture.get()});

            makeTexture(carNormalPath, *normalTexture);
            textureMap.insert({ TextureNames::carNormal, *normalTexture });

            makeTexture(carRoughnessPath, *roughTexture);
            textureMap.insert({ TextureNames::carRough, *roughTexture });

            makeTexture(carAOPath, *AOTexture);
            textureMap.insert({ TextureNames::carAO, *AOTexture });

            makeTexture(carMetalPath, *metallicTexture);
            textureMap.insert({ TextureNames::carMetal, *metallicTexture });
        }


        //meshLoad.loadModel(skullFilePath);

        //vertex_load_model = VerticesToBuffer(meshLoad.meshVertices);

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

            
            drawModel(currModel);
        

            if (PeekMessage(&msg, currWinMainData.hWnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

        }





        return (int)msg.wParam;
    }


    //void createTextures()
    //{
    //    std::string textureFolderPath = "Textures/gen/";
    //    std::string boxDiffuseName = "SpecularMap.dds";

    //    Texture textureDiffuse;
    //    makeTexture(textureFolderPath + boxDiffuseName, textureDiffuse);
    //    textureVector.push_back(textureDiffuse);
    //}



    void cleanup()
    {
        std::cout << "cleanup()\n";

        vkDeviceWaitIdle(VulkanProject::currGraphicsCard.logicalDevice);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(currGraphicsCard.logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(currGraphicsCard.logicalDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(currGraphicsCard.logicalDevice, inFlightFences[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(currGraphicsCard.logicalDevice, uniformBuffers[i], nullptr);
            vkFreeMemory(currGraphicsCard.logicalDevice, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(currGraphicsCard.logicalDevice, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(currGraphicsCard.logicalDevice, descriptorSetLayout, nullptr);

        for (auto& currTexture : textureMap)
        {
            vkDestroyImageView(currGraphicsCard.logicalDevice, currTexture.second.textureImageView, nullptr);

            vkDestroyImage(currGraphicsCard.logicalDevice, currTexture.second.textureImage, nullptr);
            vkFreeMemory(currGraphicsCard.logicalDevice, currTexture.second.textureImageMemory, nullptr);
            vkDestroySampler(currGraphicsCard.logicalDevice, currTexture.second.textureSampler, nullptr);
        }


        for (auto& mesh : currModel.meshVector)
        {

            vkDestroyBuffer(currGraphicsCard.logicalDevice, mesh.indexBuffer, nullptr);
            vkFreeMemory(currGraphicsCard.logicalDevice, mesh.indexBufferMemory, nullptr);

            vkDestroyBuffer(currGraphicsCard.logicalDevice, mesh.vertexBuffer, nullptr);
            vkFreeMemory(currGraphicsCard.logicalDevice, mesh.vertexBufferMemory, nullptr);

        }

        vkDestroyBuffer(currGraphicsCard.logicalDevice, indexBuffer, nullptr);
        vkFreeMemory(currGraphicsCard.logicalDevice, indexBufferMemory, nullptr);

        vkDestroyBuffer(currGraphicsCard.logicalDevice, vertexBuffer, nullptr);
        vkFreeMemory(currGraphicsCard.logicalDevice, vertexBufferMemory, nullptr);

        vkDestroyImageView(currGraphicsCard.logicalDevice, depthImageView, nullptr);
        vkDestroyImage(currGraphicsCard.logicalDevice, depthImage, nullptr);
        vkFreeMemory(currGraphicsCard.logicalDevice, depthImageMemory, nullptr);


        for (auto& mod : shaderModuleMap)
            vkDestroyShaderModule(currGraphicsCard.logicalDevice, mod.second, nullptr);

      
        vkDestroyCommandPool(currGraphicsCard.logicalDevice, commandPool, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(currGraphicsCard.logicalDevice, framebuffer, nullptr);
        }

        vkDestroyPipeline(currGraphicsCard.logicalDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(currGraphicsCard.logicalDevice, pipelineLayout, nullptr);
        vkDestroyRenderPass(currGraphicsCard.logicalDevice, renderPass, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(currGraphicsCard.logicalDevice, imageView, nullptr);
        }

        vkDestroySwapchainKHR(currGraphicsCard.logicalDevice, swapChain, nullptr);
        vkDestroyDevice(currGraphicsCard.logicalDevice, nullptr);

        DestroyDebugUtilsMessengerEXT(gVkInstance, debugMessenger, nullptr);
        DestroyDebugUtilsMessengerEXT(gVkInstance, Debugging::debugMessenger, nullptr);

        vkDestroySurfaceKHR(gVkInstance, win32Surface, nullptr);
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


    using namespace VulkanProject;

    setupPrototype();
    //setupDebugMessenger();
    VulkanProject::WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);

    camera.pos = defaultCameraPos;
    camera.target = defaultCameraTarget;

    camera.pos = defaultCameraPos;
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.right = glm::vec3(1.0f, 0.0f, 0.0f);
    camera.front = glm::vec3(0.0f, 0.0f, 1.0f);

    pointLight.pos = defaultLightPos;
    pointLight.color = defaultLightColor;

    //if (isDebugCallbackOutput)
    //{
    //    Debugging::PrintDebug();
    //}

    createSwapChain();
    createImageViews();
    setupRenderPass();

    createDescriptorSetLayout();

    setupGraphicsPipeline();

    createDepthResources();

    setupFrameBuffers();

    createCommandPool();



    //Loading meshes using assimp
    loadObjects();

    //createTextures();


#ifdef  DEFAULT_CUBE
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
#else
    //createVertexBufferFromVertices(currModel.meshVector[0].meshVertices, 
    //    currModel.meshVector[0].vertexBuffer, currModel.meshVector[0].vertexBufferMemory);
    //createIndexBuffer(currModel.meshVector[0].meshIndices.indexVector);

    createBuffersFromModel(currModel);

#endif //  DEFAULT_CUBE




    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    createCommandBuffers();
    createSyncObjects();

    




    //To Do: Loading textures using assimp


    UpdateWinMain();



    cleanup();
}






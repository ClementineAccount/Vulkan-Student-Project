#pragma once

#include "tinyddsloader.h"

class Texture
{
public:

    bool isLoaded = false;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    tinyddsloader::DDSFile ddsImage;

    //Some dependencies on functions in main. Created a function in main that passes by reference as a 'placeholder' until a refactor after project complete
    //void makeTexture(std::string const& filePath);
};

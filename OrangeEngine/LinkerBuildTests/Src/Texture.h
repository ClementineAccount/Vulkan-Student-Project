#pragma once

//create textures here using tinyddsloader
#include "tinyddsloader.h"
#include <string>

#include <vulkan/vulkan.hpp>

namespace VulkanProject
{
	class Texture
	{
	public:

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;

		tinyddsloader::DDSFile ddsImage;
		void makeTexture(std::string const& filePath);
	};
}

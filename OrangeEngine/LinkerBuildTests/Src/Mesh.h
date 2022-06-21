/*****************************************************************//**
 * \file   Mesh.h
 * \brief  Mesh loading class that uses assimp to read .fbx files
 * 
 * \author Clementine Shamaney, clementine.s@digipen.edu
 * \date   June 2022
 *********************************************************************/


#pragma once
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>
#include "Texture.h"

namespace VulkanProject
{
	//structure of arrays
	struct Vertices
	{
		std::vector<glm::vec3> positions;

		std::vector<glm::vec3> normals;

		std::vector<glm::vec3> tangent;
		std::vector<glm::vec3> biTangent;

		std::vector<glm::vec3> colors;


		std::vector<glm::vec2> textureCords;
	};

	struct Indices
	{
		using indicesType = uint32_t;
		std::vector<indicesType> indexVector;
	};

	struct Mesh
	{
	public:
		Vertices meshVertices;
		Indices meshIndices;

		//Buffer per mesh for now. 
		//Can think about optimization if there's a need for it but for the assignment is not necessary

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

	};



	enum class TextureType
	{
		NONE,
		DIFFUSE,
		ALBEDO,
	};

	//struct Material
	//{
	//	MaterialType type;

	//	//Do not create the memory here
	//	Texture* textureRef;
	//};

	//Model can contain many meshes 
	class Model
	{
	public:
		std::vector<Mesh> meshVector;

		//First string is the type, second string is the name
		std::unordered_map<TextureType, std::string> textureIDs;

		//noexcept to make it faster in debug mode

		void loadModel(std::string const& filePath) noexcept;
		void ProcessMesh(const aiMesh& addMesh, const aiScene& Scene) noexcept;
		void ProcessNode(const aiNode& Node, const aiScene& scene) noexcept;

		//Referenced from xGPU 
		void ImportMaterialAndTextures(const aiMaterial& Material, const aiScene& Scene, aiTextureType type);



	};
}


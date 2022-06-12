#pragma once
#include <string>
#include <vector>


#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace VulkanProject
{
	//structure of arrays
	struct Vertices
	{
		std::vector<glm::vec3> positions;

		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> colors;

		std::vector<glm::vec2> textureCords;
	};

	struct Indices
	{
		using indicesType = uint16_t;
		std::vector<indicesType> indexVector;
	};

	class Mesh
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

	//Model can contain many meshes 
	class Model
	{
	public:
		std::vector<Mesh> meshVector;

		void loadModel(std::string const& filePath);
		void ProcessMesh(const aiMesh& addMesh, const aiScene& Scene);
		void ProcessNode(const aiNode& Node, const aiScene& scene);


	};
}


#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace VulkanProject
{
	//structure of arrays
	struct Vertices
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> colors;
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

		
		void loadModel(std::string const& filePath);
	};



}


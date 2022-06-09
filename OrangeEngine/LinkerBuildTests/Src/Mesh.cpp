#include "Mesh.h"


#include <assert.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace VulkanProject
{
	void Mesh::loadModel(std::string const& filePath)
	{
		//ctrlc ctrlv: https://learnopengl.com/Model-Loading/Model
		Assimp::Importer importer;
		const aiScene* meshScene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

		//To Do: Replace this with proper runtime error handling for Release mode
		assert(meshScene->mRootNode && ("failed to load the mesh at: " + filePath).c_str());
		assert(meshScene->mMeshes && "no mesh?");

		//What if there are child meshes? You could add them to a list using bfs or dfs but we will do that later

		//"Assimp calls their vertex position array mVertices which isn't the most intuitive name."
		for (size_t i = 0; i < meshScene->mNumMeshes; ++i)
		{
			aiMesh* currMesh = meshScene->mMeshes[i];
			for (size_t j = 0; j < currMesh->mNumVertices; ++j)
			{
				meshVertices.positions.emplace_back(glm::vec3(currMesh->mVertices[j].x, currMesh->mVertices[j].y, currMesh->mVertices[j].z));
			}


			for (unsigned int i = 0; i < currMesh->mNumFaces; i++)
			{
				aiFace face = currMesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++)
					meshIndices.indexVector.push_back(face.mIndices[j]);
			}

		}

		//std::reverse(meshIndices.indexVector.begin(), meshIndices.indexVector.end());

	}
}

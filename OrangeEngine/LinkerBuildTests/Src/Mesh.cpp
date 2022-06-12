#pragma once
#include "Mesh.h"

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <assert.h>



namespace VulkanProject
{

	void Model::ProcessMesh(const aiMesh& addMesh, const aiScene& Scene)
	{
		Mesh mesh;

		//Go through the vertices to add positions, normals and so on
		for (auto i = 0u; i < addMesh.mNumVertices; ++i)
		{
			mesh.meshVertices.positions.emplace_back(addMesh.mVertices[i].x, addMesh.mVertices[i].y, addMesh.mVertices[i].z);

			//Add if there are normals
			//if (addMesh.HasNormals())
			//{
			//	mesh.meshVertices.normals.emplace_back(glm::vec3(addMesh.mNormals[i].x, addMesh.mNormals[i].y, addMesh.mNormals[i].z));
			//}

			if (addMesh.HasTextureCoords(0))
			{
				mesh.meshVertices.textureCords.emplace_back(glm::vec2(addMesh.mTextureCoords[0][i].x, addMesh.mTextureCoords[0][i].y));
			}

			if (addMesh.HasVertexColors(0))
			{
				//Only add the first color set
				mesh.meshVertices.colors.emplace_back(glm::vec3(addMesh.mColors[0][i].r, addMesh.mColors[0][i].g, addMesh.mColors[0][i].b));
			}
			else
			{
				static glm::vec3 defaultColor = glm::vec3(1.0f, 0.0f, 0.0f);
				mesh.meshVertices.colors.push_back(defaultColor);
			}
		}

		//Go through the indices
		for (auto i = 0u; i < addMesh.mNumFaces; ++i)
		{
			const auto& Face = addMesh.mFaces[i];

			for (auto j = 0u; j < Face.mNumIndices; ++j)
				mesh.meshIndices.indexVector.push_back(Face.mIndices[j]);
		}

		//std::reverse(mesh.meshIndices.indexVector.begin(), mesh.meshIndices.indexVector.end());

		meshVector.push_back(mesh);
	}

	void Model::ProcessNode(const aiNode& Node, const aiScene& Scene)
	{
		for (auto i = 0u, end = Node.mNumMeshes; i < end; ++i)
		{
			aiMesh* pMesh = Scene.mMeshes[Node.mMeshes[i]];
			ProcessMesh(*pMesh, Scene);
		}

		for (auto i = 0u; i < Node.mNumChildren; ++i)
		{
			ProcessNode(*Node.mChildren[i], Scene);
		}
	}

	void Model::loadModel(std::string const& filePath)
	{
		Assimp::Importer importer;
		const aiScene* meshScene = importer.ReadFile(filePath
			, aiProcess_Triangulate                // Make sure we get triangles rather than nvert polygons
			| aiProcess_LimitBoneWeights           // 4 weights for skin model max
			| aiProcess_GenUVCoords                // Convert any type of mapping to uv mapping
			| aiProcess_TransformUVCoords          // preprocess UV transformations (scaling, translation ...)
			| aiProcess_FindInstances              // search for instanced meshes and remove them by references to one master
			| aiProcess_CalcTangentSpace           // calculate tangents and bitangents if possible
			| aiProcess_JoinIdenticalVertices      // join identical vertices/ optimize indexing
			| aiProcess_RemoveRedundantMaterials   // remove redundant materials
			| aiProcess_FindInvalidData            // detect invalid model data, such as invalid normal vectors
			| aiProcess_PreTransformVertices       // pre-transform all vertices
			| aiProcess_FlipUVs                    // flip the V to match the Vulkans way of doing UVs
		);


		//To Do: Replace this with proper runtime error handling for Release mode
		assert(meshScene->mRootNode && ("failed to load the mesh at: " + filePath).c_str());
		assert(meshScene->mMeshes && "no mesh?");

		ProcessNode(*meshScene->mRootNode, *meshScene);

	}
}

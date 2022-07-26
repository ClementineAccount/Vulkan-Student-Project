# Vulkan-Student-Project Brief
Final Project for CSD2150 with Tomas Arce. 

This is the public repository for my final submission for a junior year graphical project for a class at DigiPen Singapore called 'Real-Time Rendering'. This project was created independently by only myself and uses relatively raw and unorgaized Vulkan C++ code, with the windows handler as only Win32 API.

Academic Goals  (Final Project Brief)
---


 - [x] FBX  mesh loader with [Assimp](https://github.com/assimp/assimp)
 - [x] Texture + Lighting
	 - [x] DDS loader support using [tinyddsloader](https://github.com/benikabocha/tinyddsloader)
	 - [x] Support the following Texture Maps
		 - [x] Albedo/Diffuse
		 - [x] Normal Map ([BC5](https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression))
		 - [x] Ambient Occlusion ([DXT1 | BC1](https://docs.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11))
		 - [x] Glossiness [Specular] ([DXT1 | BC1](https://docs.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11))
		 - [x] Roughness [Specular] ([DXT1 | BC1](https://docs.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11))
 - [x] Controllable Camera Support
 - [x] Fragment Shader Support
 - [x] [RenderDoc](https://renderdoc.org/)



References
---
The project uses the following tutorial resources for Vulkan SDK and Win32 API boilerplate code:

1. [Vulkan-Tutorial.com](https://vulkan-tutorial.com/Introduction)
2. [xGPU](https://github.com/LIONant-depot/xGPU)
3. [docs.microsoft.com](https://docs.microsoft.com/en-us/windows/win32/api/)
4. [Official Vulkan SDK](https://vulkan.lunarg.com/doc/view/1.2.148.1/windows/getting_started.html)
5. [Vulkan Programming Guide - Graham Sellers](https://www.bookdepository.com/Vulkan-Programming-Guide-Graham-Sellers/9780134464541)
6. [Vulkan Guide (For VKImage onto Swap Chain Buffer)](https://vkguide.dev/)
7. Tomas Arce CSD2150 Leture Recordings.

No other codebases were examined for this project.



Third-Party Libraries and Dependencies
---
1. [Assimp](https://github.com/assimp/assimp)
1. [Vulkan SDK](https://www.vulkan.org/)
1. [tinyddsloader](https://github.com/benikabocha/tinyddsloader)
1. [Doxygen](https://www.doxygen.nl/index.html)
1. [GLM](https://github.com/g-truc/glm)*

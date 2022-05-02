# Vulkan-Student-Project Brief
Final Project for CSD2150 with Tomas Arce. 

In addition to meeting all marking rubrics of the Final Project, this project is also an attempt to innovate in implementing DevOps techiniques for Vulkan Game Engine development, which is still an undeveloped niche among [Computer Science and Real-Time Simulation students at DigiPen Singapore](https://www.digipen.edu.sg/academics/computer-science-degrees/bs-in-computer-science-in-real-time-interactive-simulation). 

This project hence also serves as research which will lay the foundation and groundwork for successful DevOps principles that can be used to reduce the workload, crunch and human error in the GAM projects. Certain useful principles for game engine and game development such as [containerization](https://www.redhat.com/en/topics/cloud-native-apps/what-is-containerization), [cloud infrastructure](https://www.redhat.com/en/topics/cloud-computing/what-is-cloud-infrastructure) and [virtualization](https://www.xenonstack.com/insights/virtualization-in-devops/) will have to be left out of this project in the essence of time, budget and simplicity.

The automated tools will also encourage students to use kanban board and ticketing system.  
Less time spent on management means more time developing features and learning.

>~~" 'Cher, It got work on my machine, what!"~~


---

Academic Goals  (Final Project Brief)
---


 - [ ] FBX  mesh loader with [Assimp](https://github.com/assimp/assimp)
 - [ ] Texture + Lighting
	 - [ ] DDS loader support using [tinydssloader](https://github.com/benikabocha/tinyddsloader)
	 - [ ] Support the following Texture Maps
		 - [ ] Albedo/Diffuse
		 - [ ] Normal Map ([BC5](https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression))
		 - [ ] Ambient Occlusion ([DXT1 | BC1](https://docs.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11))
		 - [ ] Glossiness [Specular] ([DXT1 | BC1](https://docs.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11))
		 - [ ] Roughness [Specular] ([DXT1 | BC1](https://docs.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11))
 - [ ] Controllable Camera Support
 - [ ] Fragment Shader Support
 - [ ] Dear Imgui Editor 
	 - [ ] Mesh addition + selection and manipulation at runtime
	 - [ ] Light addition + selection and manipulation at runtime
 - [ ] Serialized scene settings (Push Constants)
	 - [ ] Diffusion Intensity
	 - [ ] Specular Intensity
	 - [ ] Ambient Lighting
	 - [ ] Camera Starting Settings
	 - [ ]  Mesh Objects
	 - [ ] Lighting Objects
 - [ ] RenderDoc
---

DevOps and Agile Goals
--

 - [ ] [CI/CD](https://www.atlassian.com/continuous-delivery/principles/continuous-integration-vs-delivery-vs-deployment) Pipeline using [GitHub Actions](https://github.com/features/actions) on a [Self-Hosted Runner](https://docs.github.com/en/actions/hosting-your-own-runners/about-self-hosted-runners)
	 - [ ] Continuous integration 
		 - [ ] Automated Building
			 - [ ] MSVC 2019 | Windows x64 | Debug
			 - [ ] MSVC 2019 | Windows x64 | Release
	 - [ ]  [Automated Testing](https://www.atlassian.com/devops/devops-tools/test-automation) with 'light' [Test-Driven Development](https://medium.com/swlh/revisiting-test-driven-development-for-a-devops-world-401f1f8d3275) principles
		 - [ ] Simple testing (error codes, data matches expected)
		 - [ ] Testing for Vulkan Validation Layers
		 - [ ] Performance Benchmark Testing
		 - [ ] (Optional) **Visual Comparison Tests***
	 - [ ] Automated Documentation
		 - [ ] Doxygen Index generation per deployment
	 - [ ] Continous 'Deployment'
		 - [ ] After building, automated packaging of compiled files, assets, documentation and dependency batch file is created and archived for each state of development, ready to be 'shipped' to Tomas at any moment.
- [ ] [GitHub Project](https://docs.github.com/en/issues/organizing-your-work-with-project-boards/managing-project-boards/about-project-boards) Kanban Board Automation
	- [ ] [Issues/Pull Requests/Merges](https://docs.github.com/en/issues/tracking-your-work-with-issues/about-issues) as trackable stories
---
*Challenging but successful implementation with scaleablity would allow graphical outputs to be checked directly against model images. This would allow graphical aspects of scenes of a project to be rigorously tested automatically, reducing workload for Quality Assurance for game development and game engine projects for this workflow. Naive implmentation may compare pixels to pixels of screenshots to spot for glaring errors.

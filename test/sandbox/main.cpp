#include <aqua/engine/Engine.h>
#include <aqua/datastructures/Tree.h>

#include <memory>
#include <iostream>

class Layer : public aqua::LayerSystem::ILayer {
public:
	virtual bool OnEvent(aqua::Event event) override {
		switch (event) {
			case aqua::Event::WINDOW_CLOSE:
				aqua::Engine::Get().StopMainLoopRequest();
				break;

			default:
				break;
		}
		return true;
	}

	virtual bool OnUpdate() override {
		return true;
	}

	virtual bool OnRender() override {
		return true;
	}
};

int main() {
	/*std::unique_ptr<aqua::Engine> engine;

	{
		aqua::EngineInfo engineInfo{};
		engineInfo.applicationName	  = "Engine Sandbox";
		engineInfo.applicationVersion = aqua::Version(0, 0, 0);
		engineInfo.renderAPI		  = aqua::RenderAPI::VULKAN;
		engineInfo.windowSystem = {
			.renderWindowTitle	  = "Engine Sandbox",
			.renderWindowSize	  = aqua::Vec2i(1280, 720),
			.windowAPI			  = aqua::WindowAPI::GLFW,
			.renderWindowEventSet = aqua::EventSet::AllEvents()
		};
		aqua::RenderPipelineCreateInfo renderPipelines[] = {
			aqua::RenderPipelineCreateInfo {
				.vertexShaderAsset = {
					.sourcePath		= "./testShader1.vert.spv",
					.reflectionPath = "./testShader1.vert.refl"
				},
				.fragmentShaderAsset = {
					.sourcePath		= "./testShader2.frag.spv",
					.reflectionPath = "./testShader2.frag.refl"
				}
			}
		};
		engineInfo.renderAPIinfo = {
			.renderPipelineCount = sizeof(renderPipelines) / sizeof(aqua::RenderPipelineCreateInfo),
			.renderPipelineCreateInfos = renderPipelines
		};

		aqua::Status engineCreateStatus = aqua::Success{};
		engine = std::make_unique<aqua::Engine>(
			engineInfo, engineCreateStatus
		);
		if (!engineCreateStatus.IsSuccess()) {
			return (int)engineCreateStatus.GetError();
		}
	}
	auto expectedLayer = aqua::LayerSystem::Get().EmplaceLayer<Layer>(aqua::EventSet::AllEvents());

	if (!expectedLayer.HasValue()) {
		std::cout << "Failed to create new layer\n";
		return -1;
	}

	engine->MainLoop();*/

	aqua::SafeRBTree<int, std::less<int>, aqua::MemorySystem::NewDeleteAllocator<int>> tree;

	for (int i = 0; i < 100; ++i) {
		tree.Emplace(i);
	}

	for (int v : tree) {
		std::cout << v << ' ';
	}

	return 0;
}
#include <aqua/engine/Engine.h>
#include <aqua/Reflection.h>

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
	aqua::Status engineCreateStatus = aqua::Success{};

	aqua::EngineInfo engineInfo{};
	engineInfo.applicationName	  = "Engine Sandbox";
	engineInfo.applicationVersion = aqua::Version(0, 0, 0);
	engineInfo.renderAPI		  = aqua::RenderAPI::VULKAN;
	engineInfo.windowSystem = {
		.renderWindowTitle    = "Engine Sandbox",
		.renderWindowSize     = aqua::Vec2i(1280, 720),
		.windowAPI			  = aqua::WindowAPI::GLFW,
		.renderWindowEventSet = aqua::EventSet::AllEvents()
	};

	std::unique_ptr<aqua::Engine> engine = std::make_unique<aqua::Engine>(
		engineInfo, engineCreateStatus
	);

	if (!engineCreateStatus.IsSuccess()) {
		return (int)engineCreateStatus.GetError();
	}
	auto expectedLayer = aqua::LayerSystem::Get().EmplaceLayer<Layer>(aqua::EventSet::AllEvents());

	if (!expectedLayer.HasValue()) {
		std::cout << "Failed to create new layer\n";
		return -1;
	}
	auto expectedReflection = aqua::DeserializeShaderReflection("testShader1.vert.refl");
	if (!expectedReflection.HasValue()) {
		std::cout << "Failed to deserialize shader reflection\n";
		return -1;
	}

	engine->MainLoop();

	return 0;
}
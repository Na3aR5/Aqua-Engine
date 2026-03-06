#include <aqua/engine/Engine.h>

#include <memory>
#include <iostream>

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
		.renderWindowEventSet = aqua::EventSet().Add(aqua::Event::WINDOW_CLOSE)
	};

	std::unique_ptr<aqua::Engine> engine = std::make_unique<aqua::Engine>(
		engineInfo, engineCreateStatus
	);

	if (!engineCreateStatus.IsSuccess()) {
		return (int)engineCreateStatus.GetError();
	}

	return 0;
}
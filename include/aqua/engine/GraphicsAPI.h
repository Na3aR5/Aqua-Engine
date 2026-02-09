#ifndef AQUA_GRAPHIC_SYSTEM_HEADER
#define AQUA_GRAPHIC_SYSTEM_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/engine/ForwardSystems.h>

namespace aqua {
	// Interface for any graphics API, which can be used in Engine
	class IGraphicsAPI {
	public:
		void ClearColorBuffer() noexcept {}
	}; // class GraphicsAPI

#if AQUA_OPENGL_GRAPHICS_API
	// OpenGL API
	class GraphicsAPI_OpenGL : public IGraphicsAPI {
	public:
		static constexpr int OPENGL_VERSION_MAJOR = 4;
		static constexpr int OPENGL_VERSION_MINOR = 5;

		static GraphicsAPI_OpenGL Get() noexcept;
		static const GraphicsAPI_OpenGL GetConst() noexcept;

	public:
		void ClearColorBuffer() noexcept;

	private:
		friend class Engine;
		friend class WindowSystem;

		static int LoadOpenGL();
	}; // class GraphicsAPI_OpenGL

	using GraphicsAPI = GraphicsAPI_OpenGL;
#endif // AQUA_OPENGL_GRAPHICS_API
} // namespace aqua

#endif // !AQUA_GRAPHIC_SYSTEM_HEADER
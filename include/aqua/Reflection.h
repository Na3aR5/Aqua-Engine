#ifndef AQUA_REFLECTION_HEADER
#define AQUA_REFLECTION_HEADER

#include <aqua/datastructures/Array.h>
#include <cstdint>

namespace aqua {
	struct ShaderReflection {
	public:
		template <typename T>
		using AllocatorType = aqua::MemorySystem::NewDeleteAllocator<T>;

	public:
		enum Stage {
			VERTEX   = 0x1,
			FRAGMENT = 0x2
		};

		enum DescriptorType {
			UNIFORM_BUFFER
		};

		struct VertexLayout {
			uint32_t location;
			uint32_t bytes;
		};

		struct DescriptorBinding {
			uint32_t       count;
			uint32_t	   set;
			uint32_t	   binding;
			uint32_t       stages;
			DescriptorType type;
			uint32_t	   bytes;
		};

		struct DescriptorSet {
			uint32_t													   set;
			SafeArray<DescriptorBinding, AllocatorType<DescriptorBinding>> bindings;
		};

		struct PushConstant {
			uint32_t offset;
			uint32_t bytes;
		};

	public:
		bool													     incomplete = true;
		aqua::SafeArray<VertexLayout, AllocatorType<VertexLayout>>	 vertexLayouts;
		aqua::SafeArray<DescriptorSet, AllocatorType<DescriptorSet>> descriptorSets;
		aqua::SafeArray<PushConstant, AllocatorType<PushConstant>>	 pushConstants;
	}; // struct ShaderReflection

	Expected<ShaderReflection, Error> DeserializeShaderReflection(const char* relfectionPath) noexcept;
} // namespace aqua

#endif // !AQUA_REFLECTION_HEADER
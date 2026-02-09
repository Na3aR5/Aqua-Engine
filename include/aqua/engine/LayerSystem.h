#ifndef AQUA_LAYER_SYSTEM_HEADER
#define AQUA_LAYER_SYSTEM_HEADER

#include <aqua/engine/EventSystem.h>
#include <aqua/datastructures/Array.h>

namespace aqua {
	template <typename LayerType>
	class LayerHandle;

	class LayerSystem {
	public:
		// Interface for layer, that describes what main application actually does (from top layer to bottom)
		class ILayer {
		public:
			virtual ~ILayer() = default;

		public:
			virtual bool OnEvent(Event) = 0;
			virtual bool OnUpdate() = 0;
			virtual bool OnRender() = 0;

		public:
			bool IsEventNeedToHandle(Event) const noexcept;
			bool IsCurrentEventBlocked() const noexcept;

			void SetEvents(const EventSet& eventSet) noexcept;

			// Do not pass current event to next layers after handling it
			void BlockCurrentEvent() noexcept;

		private:
			mutable bool m_blockCurrentEvent = false;
			EventSet     m_eventSet;
		}; // class ILayer

	public:
		static LayerSystem& Get() noexcept;
		static const LayerSystem& GetConst() noexcept;

	public:
		// Construct layer at the bottom by perfect forwarding construction arguments
		// and determine what events layer will handle
		// Return handle to constructed layer
		template <typename LayerType, typename ... Types>
		Expected<LayerHandle<LayerType>, Error> EmplaceLayer(
		const EventSet& eventSet, Types&& ... args) noexcept {
			AQUA_TRY(
				CreateUniqueData<LayerType>(std::forward<Types>(args)...),
				layer
			);
			AQUA_TRY(m_layers.EmplaceBack(std::move(layer.GetValue())), _);

			ILayer* layerPtr = m_layers.Last().GetPtr();
			layerPtr->SetEvents(eventSet);

			return LayerHandle<LayerType>(
				static_cast<unsigned int>(m_layers.GetSize() - 1),
				m_layers.Last().GetPtr()
			);
		}

		// Get layer handle by index
		template <typename LayerType>
		Expected<LayerHandle<LayerType>, Error> GetLayerHandle(unsigned int index) const noexcept {
			if (index >= m_layers.GetSize()) {
				return Unexpected<Error>(Error::ITERATOR_OR_INDEX_OUT_OF_RANGE);
			}
			return LayerHandle<LayerType>(
				index,
				m_layers.Last().GetPtr()
			);
		}

	private:
		friend class Engine;
		LayerSystem(Status& status);

	private:
		Status _HandleEvents() noexcept;
		Status _Update() noexcept;
		Status _Render() noexcept;

	private:
		SafeArray<UniqueData<ILayer>> m_layers;
	}; // class LayerSystem

	// Pointer wrapper
	template <typename LayerType>
	class LayerHandle {
	public:
		static_assert(std::is_base_of_v<LayerSystem::ILayer, LayerType>,
			"aqua::LayerHandle - LayerType must derive from LayerSystem::ILayer");

	public:
		LayerType&		 operator*()	   noexcept { return *m_ptr; }
		const LayerType& operator*() const noexcept { return *m_ptr; }

		LayerType*	     operator->()		noexcept { return m_ptr; }
		const LayerType* operator->() const noexcept { return m_ptr; }

	private:
		friend class LayerSystem;
		LayerHandle(unsigned int index, LayerSystem::ILayer* layer) :
			m_index(index), m_ptr(static_cast<LayerType*>(layer)) {}

	private:
		unsigned int m_index;
		LayerType*   m_ptr;
	}; // class LayerHandle
} // namespace aqua

#endif // !AQUA_LAYER_SYSTEM_HEADER
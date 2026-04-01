#ifndef AQUA_TREE_HEADER
#define AQUA_TREE_HEADER

#include <aqua/utility/Memory.h>
#include <aqua/utility/TemplateUtility.h>

#include <functional>

#if defined(AQUA_SUPPORT_MAP_IMPLEMENTATION)
namespace aqua {
	namespace _tree {
		class RBTreeBase {
		public:
			enum Color : bool {
				RED   = false,
				BLACK = true
			};

			enum Direction : int {
				LEFT  = 0,
				RIGHT = 1
			};

		public:
			template <typename T, typename Comp, typename Allocator>
			class TemplateBase {
			protected:
				struct _Node;

				using _NodeAllocator    = typename Allocator::template Rebind<_Node>::AllocatorType;
				using _NodePointer      = typename _NodeAllocator::Pointer;
				using _ConstNodePointer = typename _NodeAllocator::ConstPointer;

				struct _Node {
				public:
					_Node() : value() {}

					template <typename ... Types>
					_Node(Types&&... args) : value(std::forward<Types>(args)...) {}

				public:
					_NodePointer parent    = nullptr;
					_NodePointer childs[2] = {};
					Color	     color     = Color::BLACK;
					T            value;
				}; // struct _Node

				static Direction _GetDirectionFromParent(_NodePointer node) noexcept {
					return (Direction)(node->parent->childs[Direction::RIGHT] == node);
				}

				static Direction _OppositeDirection(Direction direction) noexcept {
					return (Direction)(1 - (int)direction);
				}

				static _NodePointer _MostDirection(_NodePointer node, Direction direction) noexcept {
					while (node != nullptr && node->childs[direction] != nullptr) {
						node = node->childs[direction];
					}
					return node;
				}

				static size_t _Height(_NodePointer root) noexcept {
					if (root == nullptr) {
						return 0;
					}
					size_t leftHeight = _Height(root->childs[Direction::LEFT]);
					size_t rightHeight = _Height(root->childs[Direction::RIGHT]);

					return 1 + std::max(leftHeight, rightHeight);
				}

			protected:
				template <bool IsConst = false>
				class _Iterator {
				public:
					using value_type        = T;
					using difference_type   = ptrdiff_t;
					using const_pointer     = _ConstNodePointer;
					using const_reference   = const value_type&;
					using pointer           = std::conditional_t<IsConst, _ConstNodePointer, _NodePointer>;
					using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;
					using iterator_category = std::bidirectional_iterator_tag;

				public:
					_Iterator(pointer node, pointer root) : m_node(node), m_root(root) {}

				public:
					reference       operator*()		  { return m_node->value; }
					const_reference operator*() const { return m_node->value; }

					value_type*	      operator->()		 noexcept { return &m_node->value; }
					const value_type* operator->() const noexcept { return &m_node->value; }

					_Iterator& operator++() noexcept {
						m_prev = m_node;
						if (m_node->childs[Direction::RIGHT] != nullptr) {
							m_node = _MostDirection(m_node->childs[Direction::RIGHT], Direction::LEFT);
							return *this;
						}
						while (m_node->parent != nullptr &&
							_GetDirectionFromParent(m_node) == Direction::RIGHT) {
							m_node = m_node->parent;
						}
						m_node = m_node->parent;
						return *this;
					}

					_Iterator& operator--() noexcept {
						if (m_prev != nullptr) {
							m_node = m_prev;
							m_prev = nullptr;
							return *this;
						}
						if (m_node == nullptr) {
							m_node =  _MostDirection(m_root, Direction::RIGHT);
							return *this;
						}
						if (m_node->childs[Direction::LEFT] != nullptr) {
							m_node = _MostDirection(m_node->childs[Direction::LEFT], Direction::RIGHT);
							return *this;
						}
						while (m_node->parent != nullptr &&
							_GetDirectionFromParent(m_node) == Direction::LEFT) {
							m_node = m_node->parent;
						}
						m_node = m_node->parent;
						return *this;
					}

					_Iterator operator++(int) noexcept {
						_Iterator temp{ *this };
						this->operator++();
						return temp;
					}

					_Iterator operator--(int) noexcept {
						_Iterator temp{ *this };
						this->operator--();
						return temp;
					}

					bool operator==(const _Iterator& other) const noexcept {
						return m_node == other.m_node && m_root == other.m_root;
					}

					bool operator!=(const _Iterator& other) const noexcept {
						return m_node != other.m_node && m_root == other.m_root;
					}

					operator bool() const noexcept { return m_node != nullptr; }

				public:
					_Iterator& MoveLeft() {
						m_prev = m_node;
						m_node = m_node->childs[Direction::LEFT];
						return *this;
					}

					_Iterator& MoveRight() {
						m_prev = m_node;
						m_node = m_node->childs[Direction::RIGHT];
						return *this;
					}

				private:
					pointer m_root = nullptr;
					pointer m_prev = nullptr;
					pointer m_node = nullptr;
				}; // class _Iterator
			}; // class TemplateBase
		}; // class RBTreeBase

		template <typename KeyT, typename MappedT>
		struct MapType {
		public:
			MapType() noexcept requires(std::is_nothrow_default_constructible_v<KeyT> &&
			std::is_nothrow_default_constructible_v<MappedT>) : key(), value() {}

			MapType(const KeyT& key, const MappedT& value) noexcept requires(std::is_nothrow_copy_constructible_v<KeyT> &&
			std::is_nothrow_copy_constructible_v<MappedT>) : key(key), value(value) {}

			MapType(const KeyT& key, const MappedT&& value) noexcept requires(std::is_nothrow_copy_constructible_v<KeyT> &&
				std::is_nothrow_move_constructible_v<MappedT>) : key(key), value(std::move(value)) {}

			MapType(const KeyT&& key, const MappedT& value) noexcept requires(std::is_nothrow_move_constructible_v<KeyT> &&
				std::is_nothrow_copy_constructible_v<MappedT>) : key(std::move(key)), value(value) {}

			MapType(const KeyT&& key, const MappedT&& value) noexcept requires(std::is_nothrow_move_constructible_v<KeyT> &&
				std::is_nothrow_move_constructible_v<MappedT>) : key(std::move(key)), value(std::move(value)) {}

		public:
			// compare keys only

			bool operator==(const MapType& other) const noexcept { return key == other.key; }
			bool operator==(const KeyT& other) const noexcept { return key == other; }

			bool operator>(const MapType& other) const noexcept { return key > other.key; }
			bool operator<(const MapType& other) const noexcept { return key < other.key; }
			bool operator>=(const MapType& other) const noexcept { return key >= other.key; }
			bool operator<=(const MapType& other) const noexcept { return key <= other.key; }

			bool operator>(const KeyT& other) const noexcept { return key > other; }
			bool operator<(const KeyT& other) const noexcept { return key < other; }
			bool operator>=(const KeyT& other) const noexcept { return key >= other; }
			bool operator<=(const KeyT& other) const noexcept { return key <= other; }

		public:
			KeyT    key;
			MappedT value;
		}; // struct MapType
	} // namespace _tree

	// Nothrow red-black binary tree
	template <typename ValueT, typename Comparator, typename Allocator>
	class SafeRBTree : public _tree::RBTreeBase::TemplateBase<ValueT, Comparator, Allocator> {
	public:
		using BaseType       = _tree::RBTreeBase::TemplateBase<ValueT, Comparator, Allocator>;
		using AllocatorType  = typename BaseType::_NodeAllocator;
		using ComparatorType = Comparator;
		 
		using ValueType      = ValueT;
		using Pointer        = ValueType*;
		using Reference	     = ValueType&;
		using ConstPointer   = const ValueType*;
		using ConstReference = const ValueType&;

		using Iterator      = typename BaseType::template _Iterator<false>;
		using ConstIterator = typename BaseType::template _Iterator<true>;

	private:
		using _NodePtr   = typename BaseType::_NodePointer;
		using _Color     = _tree::RBTreeBase::Color;
		using _Direction = _tree::RBTreeBase::Direction;

	public:
		SafeRBTree() noexcept : m_pair() {}
		SafeRBTree(const SafeRBTree&) = delete;
		SafeRBTree(SafeRBTree&& other) noexcept : m_pair(std::move(other.m_pair)) {
			other.m_pair.value = _ThisData();
		}

		~SafeRBTree() { Clear(); }

		SafeRBTree& operator=(const SafeRBTree&) = delete;
		SafeRBTree& operator=(SafeRBTree&& other) noexcept {
			Clear();
			m_pair = std::move(other.m_pair);
			other.m_pair.value = _ThisData();
			return *this;
		}

	public:
		template <typename ... Types>
		requires (std::is_nothrow_constructible_v<ValueType, Types...>)
		[[nodiscard]] Expected<Iterator, Error> Emplace(Types&&... args) {
			AQUA_TRY(_AllocateNode(), expectedNewNode);

			_NodePtr newNode = expectedNewNode.GetValue();
			_Create(newNode, std::forward<Types>(args)...);
			if (!_PutNode(m_pair.value.value.root, newNode)) {
				_Destroy(newNode);
				return end();
			}
			_FixInsertion(newNode);
			++m_pair.value.value.size;
			
			return _MakeIterator(newNode);
		}

		[[nodiscard]] Expected<Iterator, Error> Insert(const ValueType& value) noexcept {
			return Emplace(value);
		}

		[[nodiscard]] Expected<Iterator, Error> Insert(ValueType&& value) noexcept {
			return Emplace(std::move(value));
		}

	public:
		void Clear() noexcept {
			_Clear(m_pair.value.value.root);
			m_pair.value.value = _ThisData();
		}

	public:
		template <typename CompareType>
		ConstIterator Find(const CompareType& value) const noexcept {
			const ComparatorType& comparator = GetComparator();
			_NodePtr root = m_pair.value.value.root;

			while (root != nullptr) {
				if (value == root->value) {
					return _MakeConstIterator(root);
				}
				root = root->childs[(_Direction)(!comparator(value, root->value))];
			}
			return cend();
		}

		bool Has(const ValueType& value) const noexcept {
			return Find(value) != cend();
		}

	public:
		bool   IsEmpty() const noexcept { return m_pair.value.value.root == nullptr; }
		size_t GetSize() const noexcept { return m_pair.value.value.size; }
		size_t GetHeight() const noexcept { return this->_Height(m_pair.value.value.root); }

		ComparatorType& GetComparator() noexcept { return m_pair.value.GetCompressed(); }
		const ComparatorType& GetComparator() const noexcept { return m_pair.value.GetCompressed(); }

	public:
		Iterator      Root()			noexcept { _MakeIterator(m_pair.value.value.root); }
		ConstIterator Root()      const noexcept { _MakeConstIterator(m_pair.value.value.root); }
		ConstIterator ConstRoot() const noexcept { _MakeConstIterator(m_pair.value.value.root); }

		Iterator begin() {
			return _MakeIterator(this->_MostDirection(m_pair.value.value.root, _Direction::LEFT));
		}

		ConstIterator begin() const {
			return _MakeConstIterator(this->_MostDirection(m_pair.value.value.root, _Direction::LEFT));
		}

		ConstIterator cbegin() const {
			return _MakeConstIterator(this->_MostDirection(m_pair.value.value.root, _Direction::LEFT));
		}

		Iterator      end()		   { return _MakeIterator(nullptr); }
		ConstIterator end()  const { return _MakeConstIterator(nullptr); }
		ConstIterator cend() const { return _MakeConstIterator(nullptr); }

	private:
		void _Clear(_NodePtr node) noexcept {
			if (node == nullptr) {
				return;
			}
			if (node->childs[_Direction::LEFT] != nullptr) {
				_Clear(node->childs[_Direction::LEFT]);
			}
			if (node->childs[_Direction::RIGHT] != nullptr) {
				_Clear(node->childs[_Direction::RIGHT]);
			}
			_Destroy(node);
		}

		bool _PutNode(_NodePtr root, _NodePtr node) noexcept {
			if (root == nullptr) {
				m_pair.value.value.root = node;
				return true;
			}
			const ComparatorType& comparator = GetComparator();
			while (root) {
				if (root->value == node->value) {
					return false;
				}
				_Direction direction = (_Direction)(!comparator(node->value, root->value));
				if (root->childs[direction] == nullptr) {
					root->childs[direction] = node;
					node->parent = root;
					return true;
				}
				root = root->childs[direction];
			}
			return false;
		}

		void _FixInsertion(_NodePtr node) noexcept {
			if (node->parent == nullptr) {
				node->color = _Color::BLACK;
				return;
			}
			do {
				_NodePtr parent = node->parent;
				if (parent->color == _Color::BLACK) {
					return;
				}
				_NodePtr grand = parent->parent;
				if (grand == nullptr) { // parent is root
					parent->color = _Color::BLACK;
					return;
				}
				_Direction direction		 = this->_GetDirectionFromParent(parent);
				_Direction oppositeDirection = this->_OppositeDirection(direction);
				_NodePtr uncle = grand->childs[oppositeDirection];

				if (uncle == nullptr || uncle->color == _Color::BLACK) {
					if (node == parent->childs[oppositeDirection]) { // triangle
						node = parent;
						_Rotate(parent, direction);
						parent = node->parent;
					}
					_Rotate(grand, oppositeDirection);
					parent->color = _Color::BLACK;
					grand->color  = _Color::RED;
					return;
				}
				parent->color = _Color::BLACK;
				uncle->color  = _Color::BLACK;
				grand->color  = _Color::RED;
				node = grand;
			} while (node->parent != nullptr);

			m_pair.value.value.root->color = _Color::BLACK;
		}

		void _Rotate(_NodePtr subRoot, _Direction direction) noexcept {
			_Direction oppositeDir = this->_OppositeDirection(direction);
			_NodePtr   parent	   = subRoot->parent;
			_NodePtr   newSubRoot  = subRoot->childs[oppositeDir];
			_NodePtr   newChild    = newSubRoot->childs[direction];

			subRoot->childs[oppositeDir] = newChild;
			if (newChild != nullptr) {
				newChild->parent = subRoot;
			}
			newSubRoot->childs[direction] = subRoot;
			newSubRoot->parent = parent;
			subRoot->parent = newSubRoot;

			if (parent != nullptr) {
				_Direction subRootDirection =
					(_Direction)(parent->childs[_Direction::RIGHT] == subRoot);
				parent->childs[subRootDirection] = newSubRoot;
				return;
			}
			m_pair.value.value.root = newSubRoot;
		}

	private:
		Expected<_NodePtr, Error> _AllocateNode() noexcept {
			if constexpr (!noexcept(m_pair.GetAllocator().Allocate(1))) {
				try {
					return m_pair.GetAllocator().Allocate(1);
				}
				catch (...) {
					return Error::FAILED_TO_ALLOCATE_MEMORY;
				}
			}
			else {
				_NodePtr ptr = m_pair.GetAllocator().Allocate(1);
				if (m_pair == nullptr) {
					return Error::FAILED_TO_ALLOCATE_MEMORY;
				}
				return ptr;
			}
		}

		template <typename ... Types>
		void _Create(_NodePtr node, Types&&... args) noexcept {
			new (node) typename BaseType::_Node(std::forward<Types>(args)...);
			node->color = _Color::RED;
		}

		void _Destroy(_NodePtr node) noexcept {
			if constexpr (!std::is_trivially_destructible_v<ValueType>) {
				node->value.~ValueType();
			}
			m_pair.GetAllocator().Deallocate(node, 1);
		}

		Iterator _MakeIterator(_NodePtr node) const noexcept {
			return Iterator(node, m_pair.value.value.root);
		}

		ConstIterator _MakeConstIterator(_NodePtr node) const noexcept {
			return ConstIterator(node, m_pair.value.value.root);
		}

	private:
		struct _ThisData {
			size_t   size = 0;
			_NodePtr root = nullptr;
		};
		using _ThisDataCompressed = CompressedPair<_ThisData, ComparatorType>;

		_memory::_AllocatorPair<_ThisDataCompressed, typename BaseType::_NodeAllocator> m_pair;
	}; // class SafeRBTree
} // namespace aqua
#endif // AQUA_SUPPORT_MAP_IMPLEMENTATION

#ifdef AQUA_SUPPORT_MAP_IMPLEMENTATION
namespace aqua {
	// Nothrow map based on red-black binary tree
	template <typename KeyT, typename MappedT, typename Comparator = std::less<KeyT>,
		typename Allocator = MemorySystem::GlobalAllocator::Proxy<MappedT>>
	class SafeMap {
	public:
		using ValueType      = _tree::MapType<KeyT, MappedT>;
		using AllocatorType  = typename Allocator::template Rebind<ValueType>::AllocatorType;

		struct ComparatorType : private Comparator {
		public:
			bool operator()(const ValueType& l, const ValueType& r) const noexcept {
				return this->Comparator::operator()(l.key, r.key);
			}
			bool operator()(const KeyT& l, const ValueType& r) const noexcept {
				return this->Comparator::operator()(l, r.key);
			}
			bool operator()(const KeyT& l, const KeyT& r) const noexcept {
				return this->Comparator::operator()(l, r);
			}
		}; // struct ComparatorType

		using TreeType       = SafeRBTree<ValueType, ComparatorType, AllocatorType>;

		using KeyType        = KeyT;
		using MappedType     = MappedT;
		using Pointer        = typename AllocatorType::Pointer;
		using Reference      = ValueType&;
		using ConstPointer   = typename AllocatorType::ConstPointer;
		using ConstReference = const ValueType&;

		using Iterator       = typename TreeType::Iterator;
		using ConstIterator  = typename TreeType::ConstIterator;

	public:
		SafeMap() noexcept = default;
		SafeMap(const SafeMap&) = delete;
		SafeMap(SafeMap&& other) noexcept : m_tree(std::move(other.m_tree)) {}

		~SafeMap() = default;

		SafeMap& operator=(const SafeMap&) = delete;
		SafeMap& operator=(SafeMap&& other) noexcept {
			m_tree = std::move(other.m_tree);
			return *this;
		}

	public:
		[[nodiscard]] Expected<Iterator, Error> Emplace(KeyType&& key, MappedType&& value) noexcept {
			return m_tree.Emplace(std::forward<KeyType>(key), std::forward<MappedType>(value));
		}

		[[nodiscard]] Expected<Iterator, Error> Insert(ValueType&& value) noexcept {
			return m_tree.Emplace(std::forward<ValueType>(value));
		}

	public:
		void Clear() noexcept { m_tree.Clear(); }

		bool   IsEmpty() const noexcept { return m_tree.IsEmpty(); }
		size_t GetSize() const noexcept { return m_tree.GetSize(); }

	public:
		ConstIterator Find(const KeyType& key) const noexcept {
			return m_tree.Find(key);
		}

		bool Has(const KeyType& key) const noexcept {
			return m_tree.Find(key) != m_tree.cend();
		}

	public:
		Iterator      begin()        noexcept { return m_tree.begin(); }
		ConstIterator begin()  const noexcept { return m_tree.begin(); }
		ConstIterator cbegin() const noexcept { return m_tree.cbegin(); }

		Iterator      end()        noexcept { return m_tree.end(); }
		ConstIterator end()  const noexcept { return m_tree.end(); }
		ConstIterator cend() const noexcept { return m_tree.cend(); }

	private:
		TreeType m_tree;
	}; // class SafeMap
} // namespace aqua
#endif // AQUA_SUPPORT_MAP_IMPLEMENTATION

#endif // !AQUA_TREE_HEADER
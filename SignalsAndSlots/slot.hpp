#pragma once
#ifndef SLOT_HPP
#define SLOT_HPP

#include <utility>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <cassert>
#include <memory>

namespace my
{
	enum class slot_type
	{
		EMPTY,
		FUNCTION,
		MEMBER_FUNCTION,
		FUNCTOR
	};

	template<class... Args>
	struct slot
	{
		explicit slot() {}
		explicit slot(void(*func)(Args...))
			: current_manager_{ &strategy_function::manager }
			, current_invoker_{ &invoker_function::invoke }
		{
			current_manager_(storage_operation::CONSTRUCT, &storage_, func, nullptr, nullptr);
		}
		template<class T>
		explicit slot(T* owner, void (T::* mfunc)(Args...))
			: current_manager_{ &strategy_member_function::manager }
			, current_invoker_{ &invoker_member_function::invoke }
		{
			current_manager_(storage_operation::CONSTRUCT, &storage_, owner, nullptr, reinterpret_cast<MFunc>(mfunc));
		}
		template<class Functor>
		explicit slot(Functor func)
		{
			constexpr auto use_internal_st = use_internal_storage<Functor>::value;
			if (use_internal_st) {
				current_manager_ = &strategy_internal_functor_object<Functor>::manager;
				current_invoker_ = &invoker_internal_functor_object<Functor>::invoke;
				current_manager_(storage_operation::CONSTRUCT, &storage_, &func, nullptr, nullptr);
			}
			else {
				current_manager_ = &strategy_external_functor_object<Functor>::manager;
				current_invoker_ = &invoker_external_functor_object<Functor>::invoke;
				current_manager_(storage_operation::CONSTRUCT, &storage_, &func, nullptr, nullptr);
			}
		}
		~slot() { current_manager_(storage_operation::DESTROY, &storage_, nullptr, nullptr, nullptr); }

		slot(const slot& other)
			: current_manager_{ other.current_manager_ }
			, current_invoker_{ other.current_invoker_ }
		{
			current_manager_(storage_operation::COPY, const_cast<storage*>(&(other.storage_)), &storage_, nullptr, nullptr);
		}
		slot& operator=(const slot& other)
		{
			if (this == &other) {
				return *this;
			}

			current_manager_(storage_operation::DESTROY, &storage_, nullptr, nullptr, nullptr);
			current_manager_ = other.current_manager_;
			current_manager_(storage_operation::COPY, &(other.storage_), &storage_, nullptr, nullptr);
			current_invoker_ = other.current_invoker_;
			return *this;
		}

		slot(slot&& other) noexcept
			: current_manager_{ other.current_manager_ }
			, current_invoker_{ other.current_invoker_ }
		{
			current_manager_(storage_operation::MOVE, const_cast<storage*>(&(other.storage_)), &storage_, nullptr, nullptr);
			other.current_manager_ = &strategy_empty::manager;
		}
		slot& operator=(slot&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			current_manager_(storage_operation::DESTROY, &storage_, nullptr, nullptr, nullptr);

			current_manager_ = other.current_manager_;
			current_manager_(storage_operation::MOVE, &(other.storage_), &storage_, nullptr, nullptr);

			current_invoker_ = other.current_invoker_;

			other.current_manager_ = &strategy_empty::manager;
			other.current_invoker_ = &invoker_empty::invoke;

			return *this;
		}

		inline bool operator == (const slot& other) const noexcept 
		{ 
			bool are_equal = false;
			current_manager_(storage_operation::COMPARE, const_cast<storage*>(&storage_), 
				const_cast<storage*>(&(other.storage_)), &are_equal, nullptr);
			return are_equal;
		}
		inline bool operator != (const slot& other) const noexcept { return !(*this == other); }

		void operator () (Args... args) { current_invoker_(storage_, args...); }

		slot_type type()
		{
			if (&strategy_empty::manager == current_manager_) {
				return slot_type::EMPTY;
			}
			else if (&strategy_function::manager == current_manager_) {
				return slot_type::FUNCTION;
			}
			else if (&strategy_member_function::manager == current_manager_) {
				return slot_type::MEMBER_FUNCTION;
			}
			else {
				return slot_type::FUNCTOR;
			}
		}

	private:
		struct Obj{};
		typedef void (Obj::*MFunc)(Args...);
		struct member 
		{
			Obj* owner{ nullptr };
			MFunc mfunc{ nullptr };
		};

		using internal_storage_t = std::aligned_storage_t<4 * sizeof(void*), alignof(void*)>;
		template<class T>
		using use_internal_storage = std::bool_constant
		<
			std::is_nothrow_move_constructible_v<T> &&
			(sizeof(T) <= sizeof(internal_storage_t)) &&
			(alignof(internal_storage_t) % alignof(T) == 0)
		>;
		union storage
		{
			void(*ptr_function)(Args...);
			member mem_function;
			internal_storage_t internal_storage_functor;
			void* external_storage_functor;
		};

		enum class storage_operation
		{
			CONSTRUCT,
			DESTROY,
			COPY,
			MOVE,
			COMPARE
		};

		using function_ptr = void(*)(Args...);
		using storage_manager_ptr = void(*)(storage_operation op, void* ptr1, void* ptr2, void* ptr3, MFunc mfunc);
		using invoker_ptr = void(*)(storage&, Args...);

		class strategy_empty
		{
		public:
			static void manager(storage_operation op, void* ptr1, void* ptr2, void* ptr3, MFunc mfunc) {}
		};
		class strategy_function
		{
			static void construct_storage(storage& stor, function_ptr pfunc) { stor.ptr_function = pfunc; }
			static void destroy_storage(storage& stor) { stor.ptr_function = nullptr; }
			static void copy_storage(storage& from, storage& to) { to.ptr_function = from.ptr_function; }
			static void move_storage(storage& from, storage& to) { copy_storage(from, to); }
			static bool compare(storage& first, storage& second) { return first.ptr_function == second.ptr_function; }
		public:
			static void manager(storage_operation op, void* ptr1, void* ptr2, void* ptr3, MFunc mfunc)
			{
				switch (op)
				{
				case storage_operation::CONSTRUCT:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					construct_storage(*static_cast<storage*>(ptr1), static_cast<function_ptr>(ptr2));
				}
				return;
				case storage_operation::DESTROY:
				{
					assert(ptr1 != nullptr);
					destroy_storage(*static_cast<storage*>(ptr1));
				}
				return;
				case storage_operation::COPY:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					copy_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::MOVE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					move_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::COMPARE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					assert(ptr3 != nullptr);
					bool* are_equal = static_cast<bool*>(ptr3);
					*are_equal = compare(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				}
			}
		};
		class strategy_member_function
		{
			static void construct_storage(storage& stor, Obj* obj, MFunc pmfunc)
			{
				stor.mem_function.owner = obj;
				stor.mem_function.mfunc = pmfunc;
			}
			static void destroy_storage(storage& stor) 
			{
				stor.mem_function.owner = nullptr;
				stor.mem_function.mfunc = nullptr;
			}
			static void copy_storage(storage& from, storage& to) 
			{
				to.mem_function.owner = from.mem_function.owner;
				to.mem_function.mfunc = from.mem_function.mfunc;
			}
			static void move_storage(storage& from, storage& to) { copy_storage(from, to); }
			static bool compare(storage& first, storage& second)
			{
				return first.mem_function.owner == second.mem_function.owner &&
					first.mem_function.mfunc == second.mem_function.mfunc;
			}
		public:
			static void manager(storage_operation op, void* ptr1, void* ptr2, void* ptr3, MFunc mfunc)
			{
				switch (op)
				{
				case storage_operation::CONSTRUCT:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					assert(mfunc != nullptr);
					construct_storage(*static_cast<storage*>(ptr1), static_cast<Obj*>(ptr2), mfunc);
				}
				return;
				case storage_operation::DESTROY:
				{
					assert(ptr1 != nullptr);
					destroy_storage(*static_cast<storage*>(ptr1));
				}
				return;
				case storage_operation::COPY:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					copy_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::MOVE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					move_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::COMPARE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					assert(ptr3 != nullptr);
					bool* are_equal = static_cast<bool*>(ptr3);
					*are_equal = compare(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				}
			}
		};
		template<class Functor>
		class strategy_internal_functor_object
		{
			static void construct_storage(storage& stor, Functor func)
			{
				::new (reinterpret_cast<void*>(&(stor.internal_storage_functor))) Functor(std::move(func));
			}
			static void destroy_storage(storage& stor) 
			{ 
				(reinterpret_cast<Functor*>(&(stor.internal_storage_functor)))->~Functor();
			}
			static void copy_storage(storage& from, storage& to) 
			{
				construct_storage(to, *(reinterpret_cast<Functor*>(&(from.internal_storage_functor))));
			}
			static void move_storage(storage& from, storage& to) 
			{
				construct_storage(to, std::move(*(reinterpret_cast<Functor*>(&(from.internal_storage_functor)))));
			}
			static bool compare(storage& first, storage& second)
			{
				return &(first.internal_storage_functor) == &(second.internal_storage_functor);
			}
		public:
			static void manager(storage_operation op, void* ptr1, void* ptr2, void* ptr3, MFunc mfunc)
			{
				switch (op)
				{
				case storage_operation::CONSTRUCT:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					construct_storage(*static_cast<storage*>(ptr1), *static_cast<Functor*>(ptr2));
				}
				return;
				case storage_operation::DESTROY:
				{
					assert(ptr1 != nullptr);
					destroy_storage(*static_cast<storage*>(ptr1));
				}
				return;
				case storage_operation::COPY:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					copy_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::MOVE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					move_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::COMPARE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					assert(ptr3 != nullptr);
					bool* are_equal = static_cast<bool*>(ptr3);
					*are_equal = compare(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				}
			}
		};
		template<class Functor>
		class strategy_external_functor_object
		{
			static void construct_storage(storage& stor, Functor func)
			{
				stor.external_storage_functor = new Functor(std::move(func)); 
			}
			static void destroy_storage(storage& stor)
			{
				if (stor.external_storage_functor == nullptr) {
					return;
				}
				delete static_cast<Functor*>(stor.external_storage_functor);
				stor.external_storage_functor = nullptr;
			}
			static void copy_storage(storage& from, storage& to)
			{ 
				auto func = *(static_cast<Functor*>(from.external_storage_functor));
				construct_storage(to, std::move(func)); 
			}
			static void move_storage(storage& from, storage& to)
			{
				to.external_storage_functor = from.external_storage_functor;
				from.external_storage_functor = nullptr;
			}
			static bool compare(storage& first, storage& second)
			{
				return first.external_storage_functor == second.external_storage_functor;
			}
		public:
			static void manager(storage_operation op, void* ptr1, void* ptr2, void* ptr3, MFunc mfunc)
			{
				switch (op)
				{
				case storage_operation::CONSTRUCT:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					construct_storage(*static_cast<storage*>(ptr1), *static_cast<Functor*>(ptr2));
				}
				return;
				case storage_operation::DESTROY:
				{
					assert(ptr1 != nullptr);
					destroy_storage(*static_cast<storage*>(ptr1));
				}
				return;
				case storage_operation::COPY:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					copy_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::MOVE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					move_storage(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				case storage_operation::COMPARE:
				{
					assert(ptr1 != nullptr);
					assert(ptr2 != nullptr);
					assert(ptr3 != nullptr);
					bool* are_equal = static_cast<bool*>(ptr3);
					*are_equal = compare(*static_cast<storage*>(ptr1), *static_cast<storage*>(ptr2));
				}
				return;
				}
			}
		};


		struct invoker_empty
		{
			static void invoke(storage& stor, Args... args) 
			{
				throw std::bad_function_call{};
			}
		};
		struct invoker_function
		{
			static void invoke(storage& stor, Args... args)
			{ 
				stor.ptr_function(args...);
			}
		};
		struct invoker_member_function
		{
			static void invoke(storage& stor, Args... args)
			{
				((stor.mem_function.owner)->*(stor.mem_function.mfunc))(args...);
			}
		};
		template<class Functor>
		struct invoker_internal_functor_object
		{
			static void invoke(storage& stor, Args... args)
			{
				(*reinterpret_cast<Functor*>(&(stor.internal_storage_functor)))(args...);
			}
		};
		template<class Functor>
		struct invoker_external_functor_object
		{
			static void invoke(storage& stor, Args... args)
			{
				(*static_cast<Functor*>(stor.external_storage_functor))(args...);
			}
		};

		// members
		storage storage_{ nullptr };
		storage_manager_ptr current_manager_{ &strategy_empty::manager };
		invoker_ptr current_invoker_{ &invoker_empty::invoke };
	};
}

#endif // !SLOT_HPP

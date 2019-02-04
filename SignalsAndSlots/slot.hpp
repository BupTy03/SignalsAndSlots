#pragma once
#ifndef SLOT_HPP
#define SLOT_HPP

#include<utility>

namespace my
{
	template<class... Args>
	class slot
	{
	private:

		struct X {};
		typedef void (X::*Func)(Args...);

		X* ptr_{ nullptr };
		union
		{
			Func member_func_;
			void(*trivial_func_) (Args...);
		};
	public:

		template<class Obj>
		explicit slot(Obj* owner, void (Obj::* func)(Args...)) noexcept
			: ptr_{ reinterpret_cast<X*>(owner) }, member_func_{ reinterpret_cast<Func>(func) }
		{
			if (owner == nullptr || func == nullptr)
				throw std::invalid_argument{ "pointer to an object and pointer to member of this object expected, nullptr was given" };
		}

		explicit slot(void(*func)(Args...))
			: trivial_func_{ func }
		{
			if (func == nullptr)
				throw std::invalid_argument{ "pointer to a trivial function expected, nullptr was given" };
		}

		inline bool operator == (const slot& other) const noexcept
		{
			return this->ptr_ == other.ptr_ &&
				(this->ptr_ == nullptr && this->trivial_func_ == other.trivial_func_ ||
					this->member_func_ == other.member_func_);
		}
		inline bool operator != (const slot& other) const noexcept { return !(*this == other); }

		void operator () (Args&&... args)
		{
			if (ptr_ == nullptr)
				(*trivial_func_)(std::forward<Args>(args)...);
			else
				(ptr_->*member_func_)(std::forward<Args>(args)...);
		}
	};
}

#endif // !SLOT_HPP

#pragma once
#ifndef SLOT_HPP
#define SLOT_HPP

#include <utility>
#include <stdexcept>
#include <functional>
#include <cassert>
#include <memory>

namespace my
{
	enum class slot_type {
		NONE,
		FUNCTION,
		MEMBER_FUNCTION,
		FUNCTOR
	};

	template<class... Args>
	struct slot
	{
		slot()
			: slType_{ slot_type::NONE } {}

		explicit slot(void(*func)(Args...))
			: slType_{ slot_type::FUNCTION }
		{
			if (func == nullptr)
				throw std::invalid_argument{ "pointer to a trivial function expected, nullptr was given" };
			func_ = func;
		}

		template<class T>
		explicit slot(T* owner, void (T::* func)(Args...))
			: slType_{ slot_type::MEMBER_FUNCTION }
			, memberFunc_{ reinterpret_cast<Obj*>(owner), reinterpret_cast<MFunc>(func) }
		{
			if (owner == nullptr || func == nullptr)
				throw std::invalid_argument{ "pointer to an object and pointer to member of this object expected, nullptr was given" };
		}

		template<class Func>
		explicit slot(Func func)
			: slType_{ slot_type::FUNCTOR }
			, callable_{ std::make_shared<FunctorHolder<Func>>(func) }
		{}

		slot(const slot& other) noexcept
			: slType_{ other.slType_ }
		{
			switch (slType_)
			{
			case slot_type::FUNCTION: 
				func_ = other.func_;
				break;
			case slot_type::MEMBER_FUNCTION:
				memberFunc_ = other.memberFunc_;
				break;
			case slot_type::FUNCTOR:
				callable_ = other.callable_;
				break;
			}
		}
		slot& operator=(const slot& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			if (slType_ == slot_type::FUNCTOR) {
				callable_.reset();
			}

			slType_ = other.slType_;
			switch (slType_)
			{
			case slot_type::FUNCTION:
				func_ = other.func_;
				break;
			case slot_type::MEMBER_FUNCTION:
				memberFunc_ = other.memberFunc_;
				break;
			case slot_type::FUNCTOR:
				callable_ = other.callable_;
				break;
			}
			return *this;
		}

		slot(slot&& other) noexcept
		{
			std::swap(slType_, other.slType_);
			switch (slType_)
			{
			case slot_type::FUNCTION:
				std::swap(func_, other.func_);
				break;
			case slot_type::MEMBER_FUNCTION:
				std::swap(memberFunc_, other.memberFunc_);
				break;
			case slot_type::FUNCTOR:
				std::swap(callable_, other.callable_);
				break;
			}
		}
		slot& operator=(slot&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			if (slType_ == slot_type::FUNCTOR) {
				callable_.reset();
			}

			slType_ = other.slType_;
			switch (slType_)
			{
			case slot_type::FUNCTION:
				std::swap(func_, other.func_);
				break;
			case slot_type::MEMBER_FUNCTION:
				std::swap(memberFunc_, other.memberFunc_);
				break;
			case slot_type::FUNCTOR:
				std::swap(callable_, other.callable_);
				break;
			}
			return *this;
		}

		~slot() {}

		inline slot_type type() const noexcept { return slType_; }

		inline bool operator == (const slot& other) const noexcept 
		{ 
			if (slType_ != other.slType_) {
				return false;
			}
			switch (slType_) {
			case slot_type::NONE:
				return true;
			case slot_type::FUNCTION:
				return func_ == other.func_;
			case slot_type::MEMBER_FUNCTION:
				return memberFunc_ == other.memberFunc_;
			case slot_type::FUNCTOR:
				return callable_ == other.callable_;
			}
			return false;
		}
		inline bool operator != (const slot& other) const noexcept { return !(*this == other); }

		void operator () (Args&&... args)
		{
			switch (slType_)
			{
			case slot_type::FUNCTION:
				assert(func_ && "function is null");
				func_(std::forward<Args>(args)...);
				return;
			case slot_type::MEMBER_FUNCTION:
				assert(memberFunc_.owner && memberFunc_.mfunc && "member function is null");
				((memberFunc_.owner)->*(memberFunc_.mfunc))(std::forward<Args>(args)...);
				return;
			case slot_type::FUNCTOR:
				assert(callable_ != nullptr && "functor is null");
				callable_->call(std::forward<Args>(args)...);
				return;
			}
			throw std::bad_function_call();
		}

	private:

		struct Callable
		{
			virtual void call(Args&&...) = 0;
		};

		template<class Func>
		struct FunctorHolder final : Callable
		{
			FunctorHolder(Func f)
				: func_ { std::move(f)}
			{}

			virtual void call(Args&&... args) override { func_(std::forward<Args>(args)...); }

		private:
			Func func_;
		};

	private:
		struct Obj{};
		typedef void (Obj::*MFunc)(Args...);
		struct ObjMemberFunc
		{
			Obj* owner{ nullptr };
			MFunc mfunc{ nullptr };

			inline bool operator==(const ObjMemberFunc& other) const noexcept { return owner == other.owner && mfunc == other.mfunc; }
			inline bool operator!=(const ObjMemberFunc& other) const noexcept { !(*this == other); }
		};

	private:
		slot_type slType_;
		union
		{
			void(*func_) (Args...);
			ObjMemberFunc memberFunc_;
			std::shared_ptr<Callable> callable_;
		};
	};
}

#endif // !SLOT_HPP

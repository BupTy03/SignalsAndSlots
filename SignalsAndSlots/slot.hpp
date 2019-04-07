#pragma once
#ifndef SLOT_HPP
#define SLOT_HPP

#include <utility>
#include <stdexcept>
#include <functional>

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
		slot(){}

		explicit slot(void(*func)(Args...))
			: id_{ ++counter_ }
			, slType_{ slot_type::FUNCTION }
		{
			if (func == nullptr)
				throw std::invalid_argument{ "pointer to a trivial function expected, nullptr was given" };
			func_ = func;
		}

		template<class T>
		explicit slot(T* owner, void (T::* func)(Args...))
			: id_{++counter_}
			, slType_{ slot_type::MEMBER_FUNCTION }
		{
			if (owner == nullptr || func == nullptr)
				throw std::invalid_argument{ "pointer to an object and pointer to member of this object expected, nullptr was given" };
			memberFunc_.owner = reinterpret_cast<T*>(owner);
			memberFunc_.mfunc = reinterpret_cast<MFunc>(func);
		}

		template<class Func>
		explicit slot(Func func)
			: id_{ ++counter_ }
			, slType_{ slot_type::FUNCTOR }
			, callable_{new FunctorHolder<Func>(func)}
		{}

		slot(const slot& other) noexcept
			: id_{ other.id_ }
			, slType_{ other.slType_ }
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
				callable_ = (other.callable_).clone();
				break;
			}
		}
		slot& operator=(const slot& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			if (slType_ == slot_type::FUNCTOR) {
				delete callable_;
			}

			id_ = other.id_;
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
				callable_ = (other.callable_).clone();
				break;
			}
			return *this;
		}

		slot(slot&& other) noexcept
			: id_{ other.id_ }
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

			if (slType_ == slot_type::FUNCTOR && slType_ != other.slType_) {
				delete callable_;
			}

			id_ = other.id_;
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

		~slot()
		{
			if (slType_ == slot_type::FUNCTOR && callable_ != nullptr) {
				delete callable_;
			}
		}

		inline slot_type type() const noexcept { return slType_; }

		inline int id() const noexcept{ return id_; }

		inline bool operator == (const slot& other) const noexcept { return this->id_ == other.id_; }
		inline bool operator != (const slot& other) const noexcept { return this->id_ != other.id_; }
		inline bool operator < (const slot& other) const noexcept { return this->id_ < other.id_; }
		inline bool operator > (const slot& other) const noexcept { return this->id_ > other.id_; }
		inline bool operator <= (const slot& other) const noexcept { return this->id_ <= other.id_; }
		inline bool operator >= (const slot& other) const noexcept { return this->id_ >= other.id_; }

		void operator () (Args&&... args)
		{
			switch (slType_)
			{
			case slot_type::FUNCTION:
				func_(std::forward<Args>(args)...);
				break;
			case slot_type::MEMBER_FUNCTION:
				((memberFunc_.owner)->*mfunc)(std::forward<Args>(args)...);
				break;
			case slot_type::FUNCTOR:
				callable_->call(std::forward<Args>(args)...);
				break;
			}
		}

	private:

		struct Callable
		{
			virtual ~Callable(){}
			virtual void call(Args&&...) = 0;
			virtual Callable* clone() = 0;
		};

		template<class Func>
		struct FunctorHolder final : Callable
		{
			FunctorHolder(Func f)
				: func_{std::move(f)}
			{}

			virtual void call(Args&&... args) override { func_(std::forward<Args>(args)...); }
			virtual Callable* clone() override { return new FunctorHolder(func_); }

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
		};

	private:
		static int counter_;
		int id_{ 0 };
		slot_type slType_;
		union
		{
			void(*func_) (Args...) { nullptr };
			ObjMemberFunc memberFunc_;
			Callable* callable_{ nullptr };
		};
	};

	template<class... Args>
	int slot<Args...>::counter_ = 0;
}

#endif // !SLOT_HPP

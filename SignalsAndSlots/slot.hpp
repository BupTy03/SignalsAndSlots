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

		template<class Obj>
		explicit slot(Obj* owner, void (Obj::* func)(Args...))
			: id_{++counter_}
			, sType_{ slot_type::MEMBER_FUNCTION }
		{
			if (owner == nullptr || func == nullptr)
				throw std::invalid_argument{ "pointer to an object and pointer to member of this object expected, nullptr was given" };
			callable_ = new MemberFunctionHolder<Obj>(owner, func);
		}

		explicit slot(void(*func)(Args...))
			: id_{ ++counter_ }
			, sType_{ slot_type::FUNCTION }
		{
			if (func == nullptr)
				throw std::invalid_argument{ "pointer to a trivial function expected, nullptr was given" };
			callable_ = new FunctionHolder(func);
		}

		template<class Func>
		explicit slot(Func func)
			: id_{ ++counter_ }
			, sType_{ slot_type::FUNCTOR }
			, callable_{new FunctorHolder<Func>(func)}
		{}

		slot(const slot& other) noexcept
			: id_{ other.id_ }
			, sType_{ other.sType_ }
		{
			if (other.callable_ != nullptr) {
				callable_ = (other.callable_)->clone();
			}
		}
		slot& operator=(const slot& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			id_ = other.id_;
			sType_ = other.sType_;

			if (callable_ != nullptr) {
				delete callable_;
			}

			if (other.callable_ != nullptr) {
				callable_ = (other.callable_)->clone();
			}
			else {
				callable_ = nullptr;
			}
			return *this;
		}

		slot(slot&& other) noexcept
			: id_{ other.id_ }
			, sType_{other.sType_}
		{
			std::swap(callable_, other.callable_);
		}
		slot& operator=(slot&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			id_ = other.id_;
			sType_ = other.sType_;
			std::swap(callable_, other.callable_);
			return *this;
		}

		~slot()
		{
			if (callable_ != nullptr) {
				delete callable_;
			}
		}

		inline slot_type type() const noexcept
		{
			return sType_;
		}

		inline int id() const noexcept
		{
			return id_;
		}

		inline bool operator == (const slot& other) const noexcept
		{
			return this->id_ == other.id_;
		}
		inline bool operator != (const slot& other) const noexcept 
		{ 
			return this->id_ != other.id_;
		}
		inline bool operator < (const slot& other) const noexcept
		{
			return this->id_ < other.id_;
		}
		inline bool operator > (const slot& other) const noexcept
		{
			return this->id_ > other.id_;
		}
		inline bool operator <= (const slot& other) const noexcept
		{
			return this->id_ <= other.id_;
		}
		inline bool operator >= (const slot& other) const noexcept
		{
			return this->id_ >= other.id_;
		}

		void operator () (Args&&... args)
		{
			if (callable_ == nullptr) {
				throw std::bad_function_call();
			}
			callable_->call(std::forward<Args>(args)...);
		}

	private:

		struct Callable
		{
			virtual ~Callable(){}
			virtual void call(Args&&...) = 0;
			virtual Callable* clone() = 0;
		};

		struct FunctionHolder final : Callable
		{
			FunctionHolder(void(*f) (Args...))
				: func_{f}
			{}

			virtual void call(Args&&... args) override
			{
				if (func_ == nullptr) {
					throw std::bad_function_call();
				}
				(*func_)(std::forward<Args>(args)...);
			}

			virtual Callable* clone() override
			{
				return new FunctionHolder(func_);
			}

		private:
			void(*func_) (Args...) { nullptr };
		};

		template<class Obj>
		struct MemberFunctionHolder final : Callable
		{
			MemberFunctionHolder(Obj* owner, void (Obj::* mfunc)(Args...))
				: owner_{owner}
				, mfunc_{mfunc}
			{}

			virtual void call(Args&&... args) override
			{
				if (owner_ == nullptr || mfunc_ == nullptr) {
					throw std::bad_function_call();
				}
				(owner_->*mfunc_)(std::forward<Args>(args)...);
			}

			virtual Callable* clone() override
			{
				return new MemberFunctionHolder(owner_, mfunc_);
			}

		private:
			Obj* owner_{ nullptr };
			void (Obj::* mfunc_)(Args...) { nullptr };
		};

		template<class Func>
		struct FunctorHolder final : Callable
		{
			FunctorHolder(Func f)
				: func_{std::move(f)}
			{}

			virtual void call(Args&&... args) override
			{
				func_(std::forward<Args>(args)...);
			}

			virtual Callable* clone() override
			{
				return new FunctorHolder(func_);
			}

		private:
			Func func_;
		};

	private:
		static int counter_;
		int id_{ 0 };
		slot_type sType_;
		Callable* callable_{ nullptr };
	};

	template<class... Args>
	int slot<Args...>::counter_ = 0;
}

#endif // !SLOT_HPP

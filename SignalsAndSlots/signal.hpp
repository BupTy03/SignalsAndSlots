#pragma once
#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include "slot.hpp"
#include "connection.hpp"

#include <utility>
#include <algorithm>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <memory>

namespace my
{
	namespace internal_ns
	{
		template<class It, class T>
		inline bool contains(It first, It last, const T& val)
		{
			return last != std::find(first, last, val);
		}

		template<class Container, class T>
		inline bool contains(const Container& cont, const T& val)
		{
			return contains(std::begin(cont), std::end(cont), val);
		}

		/*template<class It, class Predicate>
		inline bool contains(It first, It last, Predicate pred)
		{
			return last != std::find(first, last, pred);
		}

		template<class Container, class Predicate>
		inline bool contains(const Container& cont, Predicate pred)
		{
			return contains(std::begin(cont), std::end(cont), pred);
		}*/
	}

	template<class... Args>
	struct signal
	{
		void operator()(Args&&... args)
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			for (std::size_t i = 0; i < slots_.size(); ++i) {
				slots_[i](std::forward<Args>(args)...);
			}
		}

		connection connect(void(*func)(Args...))
		{
			slot<Args...> sl(func);
			return connect(sl);
		}
		template<class Obj>
		connection connect(Obj* obj, void (Obj::* func)(Args...))
		{
			slot<Args...> sl(obj, func);
			return connect(sl);
		}
		connection connect(const slot<Args...>& sl)
		{
			std::unique_lock<std::shared_mutex> lock_(mx_);
			connection result(*this, sl);
			/*if (internal_ns::contains(slots_, sl)) {
				return result;
			}*/
			if (std::cend(slots_) != std::find(std::cbegin(slots_), std::cend(slots_), sl)) {
				return result;
			}
			slots_.push_back(sl);
			return result;
		}
		connection connect(slot<Args...>&& sl)
		{
			std::unique_lock<std::shared_mutex> lock_(mx_);
			connection result(*this, sl);
			/*if (internal_ns::contains(slots_, sl)) {
				return result;
			}*/
			if (std::cend(slots_) != std::find(std::cbegin(slots_), std::cend(slots_), sl)) {
				return result;
			}
			slots_.push_back(std::move(sl));
			return result;
		}

		void disconnect(std::size_t slot_id)
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			auto it = std::find_if(std::cbegin(slots_), std::cend(slots_), SlotIdPredicate(slot_id));
			/*if (std::cend(slots_) == it) {
				return;
			}*/
			slots_.erase(it);
		}
		void disconnect(void(*func)(Args...))
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			auto it = std::find_if(std::cbegin(slots_), std::cend(slots_), SlotFunctionPredicate{ func });
			/*if (std::cend(slots_) == it) {
				return;
			}*/
			slots_.erase(it);
		}
		template<class T>
		void disconnect(T* obj, void (T::*func)(Args...))
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			auto it = std::find_if(std::cbegin(slots_), std::cend(slots_), SlotMemberFunctionPredicate{ obj, func });
			/*if (internal_ns::contains(slots_, SlotMemberFunctionComparator{ obj, func })) {
				return;
			}*/
			slots_.erase(it);
		}
		void disconnect(const slot<Args...>& sl)
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			auto it = std::find(std::cbegin(slots_), std::cend(slots_), sl);
			/*if (internal_ns::contains(slots_, sl)) {
				return;
			}*/
			slots_.erase(it);
		}

	private:
		struct SlotFunctionPredicate
		{
			SlotFunctionPredicate(void(*func)(Args...))
				: func_{ func } {}

			bool operator()(const slot<Args...>& sl) const { return sl.compare(func_); }
		private:
			void(*func_)(Args...) { nullptr };
		};

		struct SlotMemberFunctionPredicate
		{
			template<class T>
			SlotMemberFunctionPredicate(T* obj, void (T::*func)(Args...))
				: owner_{ static_cast<Obj*>(obj) }, mfunc_{ reinterpret_cast<MFunc>(func) } {}

			bool operator()(const slot<Args...>& sl) const { return sl.compare(owner_, mfunc_); }
		private:
			struct Obj {};
			typedef void (Obj::*MFunc)(Args...);
			Obj* owner_{ nullptr };
			MFunc mfunc_{ nullptr };
		};

		struct SlotIdPredicate
		{
			SlotIdPredicate(std::size_t id) : id_{ id } {}

			bool operator()(const slot<Args...>& sl) const { return sl.get_id() == id_; }
		private:
			std::size_t id_{ 0 };
		};

	private:
		std::vector<slot<Args...>> slots_;
		std::shared_mutex mx_;
	};

}

#endif // !SIGNAL_HPP
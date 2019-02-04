#pragma once
#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include"slot.hpp"

#include<utility>
#include<algorithm>
#include<list>
#include<mutex>
#include<shared_mutex>

namespace my
{
	template<class... Args>
	class signal
	{
	public:

		void operator()(Args&&... args)
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			for (auto it = slots_.begin(); it != slots_.end(); )
			{
				auto tmp = it; // if the current slot is deleted,
				++it;		   // the iterator will remain valid.
				(*tmp)(std::forward<Args>(args)...);
			}
		}

		void connect(void(*func)(Args...)) const
		{
			if (func == nullptr) return;
			connect(slot<Args...>(func));
		}

		template<class Obj>
		void connect(Obj* obj, void (Obj::* func)(Args...)) const
		{
			if (obj == nullptr || func == nullptr) return;
			connect(slot<Args...>(obj, func));
		}

		void connect(const slot<Args...>& sl) const
		{
			if (std::find(std::cbegin(slots_), std::cend(slots_), sl) != std::cend(slots_))
				return;

			std::unique_lock<std::shared_mutex> lock_(mx_);
			slots_.push_back(sl);
		}

		void connect(slot<Args...>&& sl) const
		{
			if (std::find(std::cbegin(slots_), std::cend(slots_), sl) != std::cend(slots_))
				return;

			std::unique_lock<std::shared_mutex> lock_(mx_);
			slots_.push_back(std::move(sl));
		}

		void disconnect(void(*func)(Args...)) const
		{
			if (func == nullptr) return;
			disconnect(slot<Args...>(func));
		}

		template<class Obj, class... FArgs>
		void disconnect(Obj* obj, void (Obj::* func)(FArgs...)) const
		{
			disconnect(slot<FArgs...>(obj, func));
		}

		void disconnect(const slot<Args...>& sl) const
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			slots_.erase(std::find(std::begin(slots_), std::end(slots_), sl));
		}

	private:

		mutable std::list<slot<Args...>> slots_;
		mutable std::shared_mutex mx_;
	};

}

#endif // !SIGNAL_HPP
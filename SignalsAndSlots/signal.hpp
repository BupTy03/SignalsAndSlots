#pragma once
#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include"slot.hpp"

#include<utility>
#include<algorithm>
#include<vector>

namespace my
{
	template<class... Args>
	class signal
	{
	public:

		template<class... FArgs>
		void operator()(FArgs&&... args)
		{
			for (auto& sl : slots_)
			{
				sl(std::forward<FArgs>(args)...);
			}
		}

		void connect(void(*func)(Args...)) const
		{
			if (func == nullptr) return;
			connect(slot<Args...>(func));
		}

		template<class Obj, class... FArgs>
		void connect(Obj* obj, void (Obj::* func)(FArgs...)) const
		{
			if (obj == nullptr || func == nullptr) return;
			connect(slot<FArgs...>(obj, func));
		}

		void connect(const slot<Args...>& sl) const
		{
			if (std::find(std::begin(slots_), std::end(slots_), sl) != std::end(slots_))
				return;

			slots_.push_back(sl);
		}

		void connect(slot<Args...>&& sl) const
		{
			if (std::find(std::begin(slots_), std::end(slots_), sl) != std::end(slots_))
				return;

			slots_.push_back(std::move(sl));
		}

		template<class... FArgs>
		void disconnect(void(*func)(FArgs...)) const
		{
			if (func == nullptr) return;
			disconnect(slot<FArgs...>(func));
		}

		template<class Obj, class... FArgs>
		void disconnect(Obj* obj, void (Obj::* func)(FArgs...)) const
		{
			disconnect(slot<FArgs...>(obj, func));
		}

		template<class... FArgs>
		void disconnect(const slot<FArgs...>& sl) const
		{
			slots_.erase(std::find(std::begin(slots_), std::end(slots_), sl));
		}

	private:
		mutable std::vector<slot<Args...>> slots_;
	};

}

#endif // !SIGNAL_HPP
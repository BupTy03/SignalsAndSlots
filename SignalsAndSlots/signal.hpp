#pragma once
#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include "slot.hpp"
#include "connection.hpp"

#include <utility>
#include <algorithm>
#include <vector>
#include <mutex>

namespace my
{
	template<class... Args>
	struct signal
	{
		friend class connection;

		explicit signal() = default;
		~signal()
		{
			for (auto conn : connections_) {
				assert(conn != nullptr);
				conn->pSignal_ = nullptr;
			}
		}

		void operator()(Args... args) const
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			for (std::size_t i = 0; i < slots_.size(); ++i) {
				slots_[i](std::forward<Args>(args)...);
			}
		}

		connection connect(void(*func)(Args...))
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			auto it = std::find_if(std::cbegin(slots_), std::cend(slots_), 
				[func](const auto& sl) { return sl.compare(func); });

			if (std::cend(slots_) == it) {
				slot<Args...> sl(func);
				connection result(*this, sl);
				slots_.insert(std::lower_bound(std::cbegin(slots_), std::cend(slots_), sl), std::move(sl));
				return result;
			}
			else {
				return connection(*this, *it);
			}
		}
		template<class Obj>
		connection connect(Obj* obj, void (Obj::* func)(Args...))
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			auto it = std::find_if(std::cbegin(slots_), std::cend(slots_),
				[obj, func](const auto& sl) { return sl.compare(obj, func); });

			if (std::cend(slots_) == it) {
				slot<Args...> sl(obj, func);
				connection result(*this, sl);
				slots_.insert(std::lower_bound(std::cbegin(slots_), std::cend(slots_), sl), std::move(sl));
				return result;
			}
			else {
				return connection(*this, *it);
			}
		}
		connection connect(const slot<Args...>& sl)
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			connection result(*this, sl);
			auto it = std::lower_bound(std::cbegin(slots_), std::cend(slots_), sl);
			if (*it == sl) {
				return result;
			}
			slots_.insert(it, sl);
			return result;
		}
		connection connect(slot<Args...>&& sl)
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			connection result(*this, sl);
			auto it = std::lower_bound(std::cbegin(slots_), std::cend(slots_), sl);
			if (*it == sl) {
				return result;
			}
			slots_.insert(it, std::move(sl));
			return result;
		}

		void disconnect(std::size_t slot_id)
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			auto it = std::lower_bound(std::cbegin(slots_), std::cend(slots_), slot_id,
				[](const auto& sl, std::size_t id) { return sl.get_id() < id; });

			if (std::cend(slots_) != it && it->get_id() == slot_id) {
				slots_.erase(it);
			}
		}
		void disconnect(void(*func)(Args...))
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			slots_.erase(std::find_if(std::cbegin(slots_), std::cend(slots_),
				[func](const auto& sl) { return sl.compare(func); }));
		}
		template<class T>
		void disconnect(T* obj, void (T::*func)(Args...))
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			slots_.erase(std::find_if(std::cbegin(slots_), std::cend(slots_),
				[obj, func](const auto& sl) { return sl.compare(obj, func); }));
		}
		void disconnect(const slot<Args...>& sl)
		{
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			auto it = std::lower_bound(std::cbegin(slots_), std::cend(slots_), sl);
			if (it != std::cend(slots_) && *it == sl) {
				slots_.erase(it);
			}
		}

	private:
		void addConnection(connection* pConnection)
		{
			assert(pConnection != nullptr);
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			auto it = std::lower_bound(std::cbegin(connections_), std::cend(connections_), pConnection);

			if ((it == std::cend(connections_)) || (*it != pConnection)) {
				connections_.insert(it, pConnection);
			}
		}
		void deleteConnection(connection* pConnection)
		{
			assert(pConnection != nullptr);
			std::lock_guard<std::recursive_mutex> lock_(mtx_);
			auto it = std::lower_bound(std::cbegin(connections_), std::cend(connections_), pConnection);

			if ((it != std::cend(connections_)) && (*it == pConnection)) {
				connections_.erase(it);
			}
		}

	private:
		std::vector<slot<Args...>> slots_;
		std::vector<connection*> connections_;
		mutable std::recursive_mutex mtx_;
	};

}

#endif // !SIGNAL_HPP
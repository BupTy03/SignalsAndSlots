#pragma once
#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include "slot.hpp"

#include <utility>
#include <algorithm>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <memory>

namespace my
{
	template<class... Args>
	struct signal;

	struct connection
	{
		template<class... Args>
		connection(signal<Args...>& si, const slot<Args...>& sl)
			: sHolder_{ std::make_shared<SignalSlotHolder<Args...>>(si, sl) }
		{}
		connection(const connection& other)
			: sHolder_{ other.sHolder_ }
		{}
		connection& operator=(const connection& other)
		{
			if (this == &other) {
				return *this;
			}

			sHolder_ = other.sHolder_;
			return *this;
		}

		connection(connection&& other) noexcept { std::swap(sHolder_, other.sHolder_); }
		connection& operator=(connection&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}
			std::swap(sHolder_, other.sHolder_);
			return *this;
		}

		~connection() {}

		void disconnect()
		{
			if (disconnected_) {
				return;
			}
			sHolder_->disconnect_slot();
			disconnected_ = true;
		}

		struct ISignalSlotHolder
		{
			virtual void disconnect_slot() = 0;
		};

		template<class... Args>
		struct SignalSlotHolder final : ISignalSlotHolder
		{
			SignalSlotHolder(signal<Args...>& si, const slot<Args...>& sl)
				: signal_{ &si }
				, slot_{ sl }
			{}

			virtual void disconnect_slot() override { signal_->disconnect(slot_); }

		private:
			signal<Args...>* signal_;
			slot<Args...> slot_;
		};

	private:
		std::shared_ptr<ISignalSlotHolder> sHolder_;
		bool disconnected_{ false };
	};

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
			auto it = std::find(std::cbegin(slots_), std::cend(slots_), sl);
			connection result(*this, sl);
			if (it != std::cend(slots_)) {
				return result;
			}
			slots_.push_back(sl);
			return result;
		}
		connection connect(slot<Args...>&& sl)
		{
			std::unique_lock<std::shared_mutex> lock_(mx_);
			auto it = std::find(std::cbegin(slots_), std::cend(slots_), sl);
			connection result(*this, sl);
			if (it != std::cend(slots_)) {
				return result;
			}
			slots_.push_back(std::move(sl));
			return result;
		}

		void disconnect(const slot<Args...>& sl)
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			auto it = std::find(std::cbegin(slots_), std::cend(slots_), sl);
			if (it == std::cend(slots_) || *it != sl) {
				return;
			}
			slots_.erase(it);
		}

	private:
		std::vector<slot<Args...>> slots_;
		std::shared_mutex mx_;
	};

}

#endif // !SIGNAL_HPP
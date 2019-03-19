#pragma once
#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include "slot.hpp"

#include <utility>
#include <algorithm>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <cassert>

namespace my
{
	template<class... Args>
	struct signal;

	struct connection
	{
		template<class... Args>
		connection(signal<Args...>* si, const slot<Args...>& sl)
			: sHolder_{ new SignalSlotHolder<Args...>(si, sl) }
		{
			assert(si != nullptr && "nullptr was given, signal ptr required");
		}

		connection(const connection& other)
			: sHolder_{ (other.sHolder_)->clone() }
		{}
		connection& operator=(const connection& other)
		{
			if (this == &other) {
				return *this;
			}

			if (sHolder_ != nullptr) {
				delete sHolder_;
			}

			sHolder_ = (other.sHolder_)->clone();
			return *this;
		}

		connection(connection&& other) noexcept
		{
			std::swap(sHolder_, other.sHolder_);
		}
		connection& operator=(connection&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}
			std::swap(sHolder_, other.sHolder_);
			return *this;
		}

		~connection()
		{
			delete sHolder_;
		}

		void disconnect()
		{
			if (disconnected_) {
				throw std::runtime_error{ "connection is already disconnected" };
			}
			sHolder_->disconnect_slot();
			disconnected_ = true;
		}

		struct ISignalSlotHolder
		{
			virtual ~ISignalSlotHolder() {}
			virtual void disconnect_slot() = 0;
			virtual ISignalSlotHolder* clone() = 0;
		};

		template<class... Args>
		struct SignalSlotHolder final : ISignalSlotHolder
		{
			SignalSlotHolder(signal<Args...>* si, const slot<Args...>& sl)
				: signal_{ si }
				, slot_{ sl }
			{
				assert(si != nullptr && "nullptr was given, signal ptr required");
			}

			virtual void disconnect_slot() override
			{
				signal_->disconnect(slot_);
			}

			virtual ISignalSlotHolder* clone()
			{
				return new SignalSlotHolder(signal_, slot_);
			}

		private:
			signal<Args...>* signal_;
			slot<Args...> slot_;
		};

	private:
		ISignalSlotHolder* sHolder_{ nullptr };
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
			connect(sl);
			return connection(this, std::move(sl));
		}
		template<class Obj>
		connection connect(Obj* obj, void (Obj::* func)(Args...))
		{
			slot<Args...> sl(obj, func);
			connect(sl);
			return connection(this, std::move(sl));
		}
		connection connect(const slot<Args...>& sl)
		{
			std::unique_lock<std::shared_mutex> lock_(mx_);
			auto it = std::upper_bound(std::cbegin(slots_), std::cend(slots_), sl);
			slots_.insert(it, sl);
			return connection(this, sl);
		}

		void disconnect(const slot<Args...>& sl)
		{
			std::shared_lock<std::shared_mutex> lock_(mx_);
			auto it = std::lower_bound(std::cbegin(slots_), std::cend(slots_), sl);
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
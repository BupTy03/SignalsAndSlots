#pragma once
#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "slot.hpp"

#include <memory>
#include <utility>

namespace my
{
	template<class... Args>
	struct signal;

	class connection
	{
		template<class... Args>
		friend class signal;

		template<class... Args>
		connection(signal<Args...>& si, const slot<Args...>& sl)
			: pSignal_{ &si }
			, slotId_{ sl.get_id() }
			, current_disconnecter_{ &signal_slot_disconnecter<Args...>::diconnecter }
		{}

	public:
		connection(const connection& other) noexcept
			: pSignal_{ other.pSignal_ }
			, slotId_{ other.slotId_ }
			, current_disconnecter_{ other.current_disconnecter_ }
			, disconnected_{ other.disconnected_ }
		{}
		connection& operator=(const connection& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			pSignal_ = other.pSignal_;
			slotId_ = other.slotId_;
			current_disconnecter_ = other.current_disconnecter_;
			disconnected_ = other.disconnected_;
			return *this;
		}

		connection(connection&& other) noexcept { this->swap(other); }
		connection& operator=(connection&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}
			this->swap(other);
			return *this;
		}

		void disconnect()
		{
			if (disconnected_) {
				return;
			}
			current_disconnecter_(pSignal_, slotId_);
			disconnected_ = true;
		}

		void swap(connection& other)
		{
			std::swap(pSignal_, other.pSignal_);
			std::swap(slotId_, other.slotId_);
			std::swap(current_disconnecter_, other.current_disconnecter_);
			std::swap(disconnected_, other.disconnected_);
		}

		bool is_connected() { return !disconnected_; }

		template<class... Args>
		struct signal_slot_disconnecter
		{
			static void diconnecter(void* signal_ptr, std::size_t slot_id)
			{
				static_cast<signal<Args...>*>(signal_ptr)->disconnect(slot_id);
			}
		};

	private:
		void* pSignal_{ nullptr };
		std::size_t slotId_{ 0 };

		void(*current_disconnecter_)(void*, std::size_t) { nullptr };
		bool disconnected_{ false };
	};

}

#endif // !CONNECTION_HPP

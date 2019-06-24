#pragma once
#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "slot.hpp"

#include <memory>
#include <utility>
#include <algorithm>

namespace my
{
	template<class... Args>
	struct signal;

	class connection
	{
		template<class... Args>
		friend class signal;

		template<class... Args>
		connection(signal<Args...>& si, const slot<Args...>& sl) noexcept
			: pSignal_{ &si }
			, slotId_{ sl.get_id() }
			, disconnect_manager_{ &signal_slot_disconnecter<Args...>::manager }
		{}

	public:
		connection(const connection& other) noexcept
			: pSignal_{ other.pSignal_ }
			, slotId_{ other.slotId_ }
			, disconnect_manager_{ other.disconnect_manager_ }
		{}
		connection& operator=(const connection& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			pSignal_ = other.pSignal_;
			slotId_ = other.slotId_;
			disconnect_manager_ = other.disconnect_manager_;
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
			disconnect_manager_(operation::DISCONNECT, pSignal_, slotId_, nullptr);
		}

		void swap(connection& other) noexcept
		{
			std::swap(pSignal_, other.pSignal_);
			std::swap(slotId_, other.slotId_);
			std::swap(disconnect_manager_, other.disconnect_manager_);
		}

		bool is_connected() const noexcept 
		{ 
			bool flag{ false };
			disconnect_manager_(operation::IS_DISCONNECTED, pSignal_, slotId_, &flag);
			return flag;
		}

		enum class operation
		{
			IS_DISCONNECTED,
			DISCONNECT
		};

		template<class... Args>
		struct signal_slot_disconnecter
		{
			static void manager(operation op, void* signal_ptr, std::size_t slot_id, bool* flag)
			{
				assert(signal_ptr != nullptr);
				auto pSig = static_cast<signal<Args...>*>(signal_ptr);
				switch (op)
				{
				case operation::IS_DISCONNECTED:
				{
					assert(flag != nullptr);
					auto it = std::lower_bound(std::cbegin(pSig->slots_), std::cend(pSig->slots_), slot_id,
						[](const auto& sl, std::size_t id) { return sl.get_id() < id; });

					*flag = ((it == std::cend(pSig->slots_)) || (it->get_id() != slot_id));
				}
				break;
				case operation::DISCONNECT:
				{
					pSig->disconnect(slot_id);
				}
				break;
				}
			}
		};

	private:
		void* pSignal_{ nullptr };
		std::size_t slotId_{ 0 };

		void(*disconnect_manager_)(operation op, void* signal_ptr, std::size_t slot_id, bool* flag) { nullptr };
	};

}

#endif // !CONNECTION_HPP

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
		connection(signal<Args...>& si, const slot<Args...>& sl)
			: pSignal_{ &si }
			, slotId_{ sl.get_id() }
			, manager_{ &connection_manager<Args...>::manager }
		{
			manager_(operation::ADD_CONNECTION, this, pSignal_, slotId_);
		}

	public:
		~connection() { deleteConnection(); }

		connection(const connection& other) noexcept
			: pSignal_{ other.pSignal_ }
			, slotId_{ other.slotId_ }
			, manager_{ other.manager_ }
		{
			if (pSignal_ != nullptr) {
				manager_(operation::ADD_CONNECTION, this, pSignal_, slotId_);
			}
		}
		connection& operator=(const connection& other) noexcept
		{
			if (this == &other) {
				return *this;
			}

			pSignal_ = other.pSignal_;
			slotId_ = other.slotId_;
			manager_ = other.manager_;
			return *this;
		}

		connection(connection&& other) noexcept 
		{ 
			this->swap(other);
			if (pSignal_ != nullptr) {
				manager_(operation::ADD_CONNECTION, this, pSignal_, slotId_);
			}
			other.deleteConnection();
		}
		connection& operator=(connection&& other) noexcept
		{
			if (this == &other) {
				return *this;
			}
			this->swap(other);
			other.deleteConnection();
			return *this;
		}

		void disconnect()
		{
			if (is_connected()) {
				manager_(operation::DISCONNECT, this, pSignal_, slotId_);
				pSignal_ = nullptr;
			}
		}

		void swap(connection& other) noexcept
		{
			std::swap(pSignal_, other.pSignal_);
			std::swap(slotId_, other.slotId_);
			std::swap(manager_, other.manager_);
		}

		inline bool is_connected() const noexcept { return pSignal_ != nullptr; }

	private:

		enum class operation {
			ADD_CONNECTION, DELETE_CONNECTION, DISCONNECT
		};

		template<class... Args>
		struct connection_manager
		{
			static void manager(operation op, connection* conn, void* signal_ptr, std::size_t slot_id)
			{
				assert(conn != nullptr);

				auto p_sig = static_cast<signal<Args...>*>(signal_ptr);
				assert(p_sig != nullptr);

				switch (op)
				{
				case operation::ADD_CONNECTION:
				{
					p_sig->addConnection(conn);
				}
				break;
				case operation::DELETE_CONNECTION:
				{
					p_sig->deleteConnection(conn);
				}
				break;
				case operation::DISCONNECT:
				{
					p_sig->disconnect(slot_id);
					p_sig->deleteConnection(conn);
				}
				break;
				}
			}
		};

		void deleteConnection()
		{
			if (is_connected()) {
				manager_(operation::DELETE_CONNECTION, this, pSignal_, slotId_);
				pSignal_ = nullptr;
			}
		}

	private:
		void* pSignal_{ nullptr };
		std::size_t slotId_{ 0 };

		void(*manager_)(operation op, connection* conn, void* signal_ptr, std::size_t slot_id) { nullptr };
	};

}

#endif // !CONNECTION_HPP

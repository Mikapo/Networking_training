#pragma once

#include "../Utility/Net_common.h"
#include "../Utility/Thread_safe_deque.h"
#include <thread>

namespace Net
{
	template<Id_concept Id_type>
	class Net_user
	{
	public:
		using Owned_message = Owned_message<Id_type>;

	protected:
		virtual void on_notification(std::string_view notification, Severity severity = Severity::notification)
		{

		}

		Thread_safe_deque<Owned_message> m_in_queue;
		std::thread m_thread_handle;
		asio::io_context m_asio_context;
	};
};

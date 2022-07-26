#pragma once

#include "../Utility/Net_common.h"
#include "../Utility/Thread_safe_deque.h"
#include <functional>
#include <thread>

namespace Net
{
    template <Id_concept Id_type>
    class Net_user
    {
    public:
        using Owned_message = Owned_message<Id_type>;

        virtual ~Net_user() = default;

    protected:
        virtual void on_notification(std::string_view notification, Severity severity = Severity::notification)
        {
        }

        bool is_in_queue_empty()
        {
            return m_in_queue.empty();
        }

        Owned_message in_queue_pop_front()
        {
            return m_in_queue.pop_back();
        }

        void in_queue_push_back(const Owned_message& message)
        {
            m_in_queue.push_back(message);
        }

        void start_asio_thread()
        {
            if (!m_thread_handle.joinable())
            {
                if (m_asio_context.stopped())
                    m_asio_context.restart();

                m_asio_thread_stop_flag = false;
                m_thread_handle = std::thread([this] { asio_thread(); });
            }
            else
                throw std::runtime_error("asio thread is already running");
        }

        void stop_asio_thread()
        {
            if (!m_asio_thread_stop_flag)
            {
                m_asio_thread_stop_flag = true;
                m_asio_context.stop();

                if (m_thread_handle.joinable())
                    m_thread_handle.join();
            }
        }

        Protocol::socket create_socket()
        {
            return Protocol::socket(m_asio_context);
        }

        Protocol::resolver create_resolver()
        {
            return Protocol::resolver(m_asio_context);
        }

        Protocol::acceptor create_acceptor(const Protocol::endpoint& endpoint)
        {
            return Protocol::acceptor(m_asio_context, endpoint);
        }

        void give_asio_job(std::function<void()> job)
        {
            if (!m_asio_thread_stop_flag)
                asio::post(m_asio_context, job);
            else
                throw std::runtime_error("asio thread was not running");
        }

    private:
        void asio_thread()
        {
            while (!m_asio_thread_stop_flag)
                m_asio_context.run();
        }

        std::thread m_thread_handle;
        asio::io_context m_asio_context;
        Thread_safe_deque<Owned_message> m_in_queue;
        bool m_asio_thread_stop_flag = true;
    };
}; // namespace Net

#include "Net_framework.h"
#include <iostream>
#include <string>

enum class Message_id : uint8_t
{
    set_name = 0,
    message = 1,
    server_message = 3
};

class Chat_client : public Net::Client_interface<Message_id>
{
private:
    void on_message(Net::Net_message<Message_id>& message) override
    {
        switch (message.m_header.m_id)
        {
        case Message_id::server_message: {
            const std::string string = message.extract_as_string();
            std::cout << string << "\n";
            std::cout.flush();
        }

        default:
            break;
        }
    }
};

bool send_thread_exit_flag = false;
Net::Thread_safe_deque<Net::Net_message<Message_id>> messages;

void send_thread()
{
    while (!send_thread_exit_flag)
    {
        std::string message;
        std::getline(std::cin, message);

        Net::Net_message<Message_id> net_message;
        net_message.m_header.m_id = Message_id::message;
        net_message.push_back(message.begin(), message.end());

        messages.push_back(net_message);
    }
}

void send_name(Chat_client& client)
{
    std::cout << "Please enter your username: ";
    std::string username;
    std::getline(std::cin, username);

    Net::Net_message<Message_id> message;
    message.m_header.m_id = Message_id::set_name;
    message.push_back(username.begin(), username.end());

    client.send_message(message);
}

void main_loop(Chat_client& client)
{
    while (client.is_connected())
    {
        client.handle_received_messages();

        if (!messages.empty())
            client.send_message(messages.pop_front());
    }
}

int main()
{
    std::cout << "Write server ip: ";
    std::string ip;
    std::getline(std::cin, ip);
    std::cout << "Write server port: ";
    std::string port;
    std::getline(std::cin, port);

    Chat_client client;
    client.connect(ip, port);

    if (client.is_connected())
    {
        std::cout << "Connected succefully \n";
        send_name(client);

        std::thread thread = std::thread(send_thread);

        main_loop(client);

        send_thread_exit_flag = true;

        if (thread.joinable())
            thread.join();
    }
    else
    {
        std::cout << "failed to connect \n";
        return 1;
    }

    std::cout << "Lost connection to server \n";
    std::cout << "Press enter to exit... \n";
    std::cin.get();
}

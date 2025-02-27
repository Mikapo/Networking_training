#pragma once

#include "../Utility/Common.h"
#include "Message_header.h"
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Net
{
    // This class is used to store messages that are sent over internet
    template <Id_concept Id_type>
    class Message
    {
    public:
        // Type used to store container sizes in the message body
        using Size_type = uint64_t;

        // Print operator
        friend std::ostream& operator<<(std::ostream& stream, const Message& message)
        {
            stream << "ID: " << static_cast<std::underlying_type_t<Id_type>>(message.m_header.m_id)
                   << " Size: " << message.m_header.m_size;
            return stream;
        }

        // Copies buffer to the end of the message
        void push_back_buffer(const void* buffer, size_t buffer_size)
        {
            const size_t size = m_body.size();
            const size_t new_size = size + buffer_size;

            if (new_size > std::numeric_limits<Header_size_type>::max())
                throw std::length_error("Storing too much data to message");

            resize_body(new_size);

            // Copying the data to the end of the body
            std::memcpy(&m_body.at(size), buffer, buffer_size);

            m_header.m_size = checked_cast<Header_size_type>(m_body.size());
        }

        /**
         *   Push data to the end of the message
         *
         *   @param data to be pushed
         *   @throws if the size of data is larger than the max value that the Header_size_type can hold
         */
        template <typename Data_type>
        void push_back(const Data_type& data)
        {
            static_assert(std::is_standard_layout_v<Data_type>);
            push_back_buffer(&data, sizeof(data));
        }

        template <>
        void push_back<std::string_view>(const std::string_view& string)
        {
            push_back_buffer(string.data(), string.size());
            push_back(static_cast<Size_type>(string.size()));
        }

        template <>
        void push_back<std::string>(const std::string& string)
        {
            push_back<std::string_view>(string);
        }

        // Extract to the buffer from the end of the message
        void extract_to_buffer(void* buffer, size_t buffer_size)
        {
            if (buffer_size > m_body.size())
                throw std::length_error("Not enough data to extract");

            const size_t new_size = m_body.size() - buffer_size;

            // Copying to the buffer from the end of the body
            std::memcpy(buffer, &m_body.at(new_size), buffer_size);

            resize_body(new_size);
            m_header.m_size = checked_cast<Header_size_type>(m_body.size());
        }

        /**
         *   Extract data from the end of the message
         *
         *   @return the data that was extracted
         *   @throws if there is not enough data to be extracted
         */
        template <typename Data_type>
        Data_type extract()
        {
            static_assert(std::is_standard_layout_v<Data_type>);

            constexpr static size_t data_size = sizeof(Data_type);
            std::array<char, data_size> buffer;
            extract_to_buffer(buffer.data(), data_size);
            return *reinterpret_cast<Data_type*>(buffer.data());
        }

        template <>
        std::string extract<std::string>()
        {
            std::string output;
            output.resize(extract<Size_type>());
            extract_to_buffer(output.data(), output.size());
            return output;
        }

        // Operator << for pushing data
        template <typename Data_type>
        friend Message& operator<<(Message& message, const Data_type& data)
        {
            message.push_back<Data_type>(data);
            return message;
        }

        // Operator >> for extracting data
        template <typename Data_type>
        friend Message& operator>>(Message& message, Data_type& data)
        {
            data = message.extract<Data_type>();
            return message;
        }

        [[nodiscard]] bool operator==(const Message& other) const noexcept
        {
            return m_header == other.m_header && m_body == other.m_body;
        }

        [[nodiscard]] bool operator!=(const Message& other) const noexcept
        {
            return !(*this == other);
        }

        /**
         *   Sets internal id of the message.
         *   This should never be called outside the framework.
         */
        void set_internal_id(Internal_id new_internal_id) noexcept
        {
            m_header.m_internal_id = new_internal_id;
        }

        // Outside of framework this should always be not_internal
        [[nodiscard]] Internal_id get_internal_id() const noexcept
        {
            return m_header.m_internal_id;
        }

        [[nodiscard]] Id_type get_id() const noexcept
        {
            return m_header.m_id;
        }

        void set_id(Id_type new_id) noexcept
        {
            m_header.m_id = new_id;
        }

        void clear() noexcept
        {
            m_body.clear();
            m_header.m_size = 0;
        }

        [[nodiscard]] bool is_empty() const noexcept
        {
            return m_body.empty();
        }

        [[nodiscard]] const Message_header<Id_type>& get_header() const noexcept
        {
            return m_header;
        }

        [[nodiscard]] size_t header_size() const noexcept
        {
            return sizeof(Message_header<Id_type>);
        }

        [[nodiscard]] auto* header_data(this auto& self) noexcept
        {
            return &self.m_header;
        }

        [[nodiscard]] size_t body_size() const noexcept
        {
            return m_body.size();
        }

        [[nodiscard]] auto* body_data(this auto& self) noexcept
        {
            return self.m_body.data();
        }

        void resize_body(size_t new_size)
        {
            m_body.resize(new_size);
        }

    private:
        // Simple integral cast with check that the value has not changes after cast
        template <std::integral Cast_to, std::integral Cast_from>
        static Cast_to checked_cast(Cast_from value)
        {
            Cast_to casted_value = static_cast<Cast_to>(value);

            if (value != casted_value)
                throw std::length_error("Value changed when casted");

            return casted_value;
        }

        Message_header<Id_type> m_header;

        // The message body in bytes
        std::vector<char> m_body = {};
    };
} // namespace Net

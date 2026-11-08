/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ByteWriter.h
*/

#ifndef BYTEWRITER_H
#define BYTEWRITER_H

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "Protocol.h"

namespace RType::Network {
/**
 * @brief Appends values to a byte buffer with the protocol encoding:
 *        little-endian integers, IEEE 754 floats, length-prefixed
 *        strings and byte arrays.
 */
class ByteWriter {
public:
    template<WireInteger T>
    void write(T value)
    {
        auto raw =
            static_cast<std::uint64_t>(static_cast<wire_raw_t<T>>(value));

        for (std::size_t i = 0; i < sizeof(T); ++i) {
            _buffer.push_back(static_cast<std::uint8_t>(raw >> (i * 8)));
        }
    }

    void writeFloat(float value);

    /**
     * @brief Writes a u8 length followed by the characters.
     * @throws std::length_error If @p str is longer than MAX_STRING_SIZE.
     */
    void writeString(std::string_view str);

    /**
     * @brief Writes a u16 length followed by the bytes.
     * @throws std::length_error If @p bytes is longer than 65535 bytes.
     */
    void writeBytes(std::span<const std::uint8_t> bytes);

    /** @brief Appends @p bytes as is, without length prefix. */
    void writeRaw(std::span<const std::uint8_t> bytes);

    [[nodiscard]] const std::vector<std::uint8_t> &data() const;

    /** @brief Moves the buffer out, leaving the writer empty. */
    std::vector<std::uint8_t> release();

private:
    std::vector<std::uint8_t> _buffer;
};
} // namespace RType::Network

#endif // BYTEWRITER_H

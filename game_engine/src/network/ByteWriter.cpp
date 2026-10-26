/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ByteWriter.cpp
*/

#include "ByteWriter.h"

#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace RType::Network {
void ByteWriter::writeFloat(float value)
{
    write(std::bit_cast<std::uint32_t>(value));
}

void ByteWriter::writeString(std::string_view str)
{
    if (str.size() > MAX_STRING_SIZE) {
        throw std::length_error("ByteWriter: string longer than 255 bytes");
    }
    write(static_cast<std::uint8_t>(str.size()));
    _buffer.insert(_buffer.end(), str.begin(), str.end());
}

void ByteWriter::writeBytes(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() > std::numeric_limits<std::uint16_t>::max()) {
        throw std::length_error("ByteWriter: byte array too long");
    }
    write(static_cast<std::uint16_t>(bytes.size()));
    writeRaw(bytes);
}

void ByteWriter::writeRaw(std::span<const std::uint8_t> bytes)
{
    _buffer.insert(_buffer.end(), bytes.begin(), bytes.end());
}

const std::vector<std::uint8_t> &ByteWriter::data() const
{
    return _buffer;
}

std::vector<std::uint8_t> ByteWriter::release()
{
    return std::exchange(_buffer, {});
}
} // namespace RType::Network

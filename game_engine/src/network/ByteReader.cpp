/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ByteReader.cpp
*/

#include "ByteReader.h"

#include <bit>

namespace RType::Network {
    ByteReader::ByteReader(std::span<const std::uint8_t> data) :
        _data(data)
    {
    }

    float ByteReader::readFloat()
    {
        return std::bit_cast<float>(read<std::uint32_t>());
    }

    std::string ByteReader::readString()
    {
        auto size = read<std::uint8_t>();

        if (!canRead(size)) {
            return {};
        }
        auto chars = _data.subspan(_offset, size);

        _offset += size;
        return {chars.begin(), chars.end()};
    }

    std::vector<std::uint8_t> ByteReader::readBytes()
    {
        auto size = read<std::uint16_t>();

        if (!canRead(size)) {
            return {};
        }
        auto bytes = _data.subspan(_offset, size);

        _offset += size;
        return {bytes.begin(), bytes.end()};
    }

    void ByteReader::fail()
    {
        _failed = true;
    }

    bool ByteReader::ok() const
    {
        return !_failed;
    }

    std::size_t ByteReader::remaining() const
    {
        return _data.size() - _offset;
    }

    bool ByteReader::atEnd() const
    {
        return _offset == _data.size();
    }

    bool ByteReader::canRead(std::size_t size)
    {
        if (_failed || size > remaining()) {
            _failed = true;
            return false;
        }
        return true;
    }
} // namespace RType::Network

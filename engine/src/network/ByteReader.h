/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ByteReader.h
*/

#ifndef BYTEREADER_H
#define BYTEREADER_H

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "Protocol.h"

namespace RType::Network {
/**
 * @brief Reads values written by ByteWriter from untrusted bytes.
 *
 * Every read is bounds-checked. Reading past the end does not throw: it
 * returns a default value and puts the reader in a failed state, which
 * stays until the reader is destroyed. Check ok() once after decoding a
 * whole payload.
 */
class ByteReader {
public:
    explicit ByteReader(std::span<const std::uint8_t> data);

    template<WireInteger T>
    T read()
    {
        if (!canRead(sizeof(T))) {
            return T {};
        }
        std::uint64_t raw = 0;

        for (std::size_t i = 0; i < sizeof(T); ++i) {
            auto byte = static_cast<std::uint64_t>(_data[_offset + i]);

            raw |= byte << (i * 8);
        }
        _offset += sizeof(T);
        return static_cast<T>(static_cast<wire_raw_t<T>>(raw));
    }

    float readFloat();

    /** @brief Reads a u8 length followed by the characters. */
    std::string readString();

    /** @brief Reads a u16 length followed by the bytes. */
    std::vector<std::uint8_t> readBytes();

    /**
     * @brief Puts the reader in the failed state, for values that were
     *        read correctly but are invalid (e.g. unknown enum value).
     */
    void fail();

    /** @return false if a read went out of bounds or fail() was called. */
    [[nodiscard]] bool ok() const;

    [[nodiscard]] std::size_t remaining() const;

    [[nodiscard]] bool atEnd() const;

private:
    bool canRead(std::size_t size);

    std::span<const std::uint8_t> _data;
    std::size_t _offset = 0;
    bool _failed = false;
};
} // namespace RType::Network

#endif // BYTEREADER_H

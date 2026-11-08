/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Endpoint.h
*/

#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <compare>
#include <cstdint>
#include <string>

namespace RType::Network {
/**
 * @brief Address and port of a peer.
 *
 * Used instead of the Asio endpoint so that no code outside UdpSocket.cpp
 * depends on Asio. The server identifies a client by its endpoint
 * (docs/protocol.md, section 2), hence the ordering for use as a map key.
 */
struct Endpoint {
    std::string address;
    std::uint16_t port = 0;

    /** @return "address:port", for logs. */
    [[nodiscard]] std::string toString() const
    {
        return address + ":" + std::to_string(port);
    }

    auto operator<=>(const Endpoint &other) const = default;
};
} // namespace RType::Network

#endif // ENDPOINT_H

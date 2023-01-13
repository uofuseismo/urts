#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_CHECK_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_CHECK_RESPONSE_HPP
#include <vector>
#include <string>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
namespace
{

[[nodiscard]]
std::pair<std::chrono::microseconds, std::chrono::microseconds>
checkPacketsAndGetStartEndTime(
    const std::vector<URTS::Broadcasts::Internal::DataPacket::DataPacket>
        &packets)
{
    if (packets.empty())
    {
        throw std::runtime_error("No packets - shouldn't be here");
    }
    constexpr std::chrono::microseconds tMin{-2208988777000000}; // 1900
    constexpr std::chrono::microseconds tMax{ 7258118423000000}; // 2200
    auto t0Packets = tMax;
    auto t1Packets = tMin;
    auto network = packets.at(0).getNetwork();
    auto station = packets.at(0).getStation();
    auto channel = packets.at(0).getChannel();
    auto locationCode = packets.at(0).getLocationCode();
    for (const auto &packet : packets)
    {
        t0Packets = std::min(t0Packets, packet.getStartTime());
        t1Packets = std::max(t1Packets, packet.getEndTime());
        if (!packet.haveSamplingRate())
        {
            throw std::invalid_argument("Sampling rate not set for packet ");
        }
        if (network != packet.getNetwork())
        {
            throw std::invalid_argument("Inconsistent network codes");
        }
        if (station != packet.getStation())
        {
            throw std::invalid_argument("Inconsistent station names");
        }
        if (channel != packet.getChannel())
        {
            throw std::invalid_argument("Inconsistent channel codes");
        }
        if (locationCode != packet.getLocationCode())
        {
            throw std::invalid_argument("Inconsistent location codes");
        }
    }
    if (t0Packets == tMax || t1Packets == tMin)
    {
        throw std::runtime_error("Failed to get time limits");
    }
    return std::pair {t0Packets, t1Packets};
}

}
#endif

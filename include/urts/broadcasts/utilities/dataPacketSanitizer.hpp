#ifndef URTS_BROADCASTS_UTILITIES_DATA_PACKET_SANITIZER_HPP
#define URTS_BROADCASTS_UTILITIES_DATA_PACKET_SANITIZER_HPP
#include <memory>
#include <umps/logging/log.hpp>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
namespace URTS::Broadcasts::Internal::DataPacket
{
 class DataPacket;
}
namespace URTS::Broadcasts::Utilities
{
/// @brief This utility is run before the sending data packets to a broadcast.
///        This utility can:
///        (1) Prevent future packets from being broadcast
///        (2) Prevent very old packets from being broadcast
///        (3) Prevent duplicate packets from being broadcast
class DataPacketSanitizer
{
public:
    DataPacketSanitizer();
    explicit DataPacketSanitizer(std::shared_ptr<UMPS::Logging::ILog> &logger);
    DataPacketSanitizer(const DataPacketSanitizer &sanitizer);
    DataPacketSanitizer(DataPacketSanitizer &&sanitizer) noexcept;
    /// @result True indicates the packet should be allowed to be broadcast.
    [[nodiscard]] bool allow(const URTS::Broadcasts::Internal::DataPacket::DataPacket &packet);
    ~DataPacketSanitizer();

    DataPacketSanitizer &operator=(const DataPacketSanitizer &sanitizer);
    DataPacketSanitizer &operator=(DataPacketSanitizer &&sanitizer) noexcept;
private:
    class DataPacketSanitizerImpl;
    std::unique_ptr<DataPacketSanitizerImpl> pImpl;  
};
}
#endif

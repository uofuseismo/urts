#ifndef URTS_BROADCASTS_UTILITIES_DATA_PACKET_SANITIZER_HPP
#define URTS_BROADCASTS_UTILITIES_DATA_PACKET_SANITIZER_HPP
#include <set>
#include <memory>
#include <umps/logging/log.hpp>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
namespace URTS::Broadcasts::Internal::DataPacket
{
 class DataPacket;
}
namespace URTS::Broadcasts::Utilities
{
 class DataPacketSanitizerOptions;
}
namespace URTS::Broadcasts::Utilities
{
/// @class DataPacketSanitizer dataPacketSanitizer.hpp "urts/broadcasts/utilities/dataPacketSanitizer.hpp"
/// @brief This utility is run before the sending data packets to a broadcast.
///        This utility can:
///        (1) Prevent future packets from being broadcast
///        (2) Prevent very old packets from being broadcast
///        (3) Prevent duplicate packets from being broadcast
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class DataPacketSanitizer
{
public:
    /// @brief Defines the data problem.
    enum class Problem
    {
        FutureData,    /*!< The data is from the future (bad timing). */
        DuplicateData, /*!< This is a duplicate data packet. */
        BadTiming,     /*!< It appears the GPS clock slipped a bit. */
        ExpiredData,   /*!< The data packet was too old to be propagated. */
        EmptyData      /*!< An empty data packet was received. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    DataPacketSanitizer();
    /// @brief Constructor with a logger.
    explicit DataPacketSanitizer(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Copy constructor.
    /// @param[in] sanitizer  The sanitizer from which to initialize this class.
    DataPacketSanitizer(const DataPacketSanitizer &sanitizer);
    /// @brief Move constructor.
    /// @param[in,out] sanitizer  The sanitizer from which to in initialize this
    ///                           class.  On exit, sanitizer's behavior is
    ///                           undefined.
    DataPacketSanitizer(DataPacketSanitizer &&sanitizer) noexcept;
    /// @}

    /// @name Initialization
    /// @{

    /// @brief Initializes the class.
    void initialize(const DataPacketSanitizerOptions &options);
    /// @}

    /// @param[in] packet  The data packet to check if it should be allowed
    ///                    to continue into the data broadcast.
    /// @result True indicates the packet should be allowed to be broadcast.
    [[nodiscard]] bool allow(const URTS::Broadcasts::Internal::DataPacket::DataPacket &packet);
    /// @result Gets the channels that have experienced the desired
    ///         error in the past bad data logging interval.
    /// @param[in] problem   The problem category.
    [[nodiscard]] std::set<std::string> getBadChannels(const Problem problem) const noexcept;

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~DataPacketSanitizer();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    DataPacketSanitizer &operator=(const DataPacketSanitizer &sanitizer);
    /// @brief Move assignment operator.
    DataPacketSanitizer &operator=(DataPacketSanitizer &&sanitizer) noexcept;
    /// @}
private:
    class DataPacketSanitizerImpl;
    std::unique_ptr<DataPacketSanitizerImpl> pImpl;  
};
}
#endif

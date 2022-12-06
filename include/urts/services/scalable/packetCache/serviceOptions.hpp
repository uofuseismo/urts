#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_SERVICE_OPTIONS_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_SERVICE_OPTIONS_HPP
#include <memory>
#include <chrono>
namespace UMPS::Authentication
{
 class ZAPOptions;
}
namespace URTS::Broadcasts::Internal::DataPacket
{
 class SubscriberOptions;
}
namespace URTS::Services::Scalable::PacketCache
{
/// @class ServiceOptions "serviceOptions.hpp" "urts/services/scalable/packetCache/serviceOptions.hpp"
/// @brief The options that define the backend packet cache service.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ServiceOptions
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ServiceOptions();
    /// @brief Copy constructor.
    /// @param[in] options  The options from which to initialize
    ///                        this class. 
    ServiceOptions(const ServiceOptions &options);
    /// @brief Move constructor.
    /// @param[in] options  The options from which to initialize this
    ///                        class.  On exit, option's behavior is
    ///                        undefined. 
    ServiceOptions(ServiceOptions &&options) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] options   The options class to copy to this.
    /// @result A deep copy of options.
    ServiceOptions& operator=(const ServiceOptions &options);
    /// @brief Move assignment.
    /// @param[in,out] options  The options whose memory will be moved to
    ///                          this.  On exit, options's behavior is
    ///                          undefined.
    /// @result The memory from options moved to this.
    ServiceOptions& operator=(ServiceOptions &&options) noexcept;
    /// @}

    /// @brief Loads the options from an initialization file.
    /// @param[in] fileName   The name of the initialization file.
    /// @param[in] section    The section of the initialization file with the
    ///                       packet cache service options to be parsed.
    /// @throws std::invalid_argument if the initialization file does not,
    ///         exist cannot be parsed, does not have the specified section,
    ///         or has incorrect information.
    void parseInitializationFile(const std::string &fileName,
                                 const std::string &section = "PacketCache");

    /// @name Data Packet Broadcast Subscriber Options
    /// @{

    /// @brief Sets the data packet broadcast subscriber options.
    /// @param[in] options  The data packet broadcast subscriber options.
    /// @throws std::invalid_argument if the address is not set.
    void setDataPacketSubscriberOptions(
        const URTS::Broadcasts::Internal::DataPacket::SubscriberOptions &options);
    /// @result The data packet broadcast subscriber options.
    /// @throws std::runtime_error if \c haveDataPacketSubscriberOptions()
    ///         is false.
    [[nodiscard]] URTS::Broadcasts::Internal::DataPacket::SubscriberOptions
        getDataPacketSubscriberOptions() const;
    /// @result True indicates the data packet subscriber options were set.
    [[nodiscard]] bool haveDataPacketSubscriberOptions() const noexcept;
    /// @}

    /// @name Replier Required Options
    /// @{
 
    /// @brief Sets the replier address to which to connect.
    /// @param[in] address  The address to which this backend service will
    ///                     connect.
    void setReplierAddress(const std::string &address);
    /// @result The replier address to which to connect.
    /// @throws std::runtime_error if \c haveBackendAddress() is false.
    [[nodiscard]] std::string getReplierAddress() const;
    /// @result True indicates the replier address was set.
    [[nodiscard]] bool haveReplierAddress() const noexcept;
    /// @} 

    /// @name Replier Optional Options
    /// @{

    /// @brief Sets the ZeroMQ Authentication Protocol options.
    /// @param[in] zapOptions  The ZAP options for the replier.
    void setReplierZAPOptions(
        const UMPS::Authentication::ZAPOptions &zapOptions) noexcept;
    /// @result The ZAP options.
    [[nodiscard]] UMPS::Authentication::ZAPOptions getReplierZAPOptions() const noexcept;

    /// @brief To receive replies we poll on a socket.  After this amount of
    ///        time has elapsed the process can proceed and handle other
    ///        essential activities like if the program has stopped.  If this
    ///        is too small then the thread will needlessly burn cycles on a
    ///        while loop but if its too big then thread will be unresponsive
    ///        on shut down.
    /// @param[in] timeOut  The time to wait for a request before the thread
    ///                     checks other things.
    /// @throws std::invalid_argument if this is negative.
    void setReplierPollingTimeOut(const std::chrono::milliseconds &timeOut);
    /// @result The polling time out.
    [[nodiscard]] std::chrono::milliseconds getReplierPollingTimeOut() const noexcept;
    /// @brief Influences the maximum number of request messages to cache
    ///        on the socket.
    /// @param[in] highWaterMark  The approximate max number of messages to 
    ///                           cache on the socket.  0 will set this to
    ///                           "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setReplierReceiveHighWaterMark(int highWaterMark);
    /// @result The high water mark.  The default is 0.
    [[nodiscard]] int getReplierReceiveHighWaterMark() const noexcept;
    /// @brief Influences the maximum number of response messages to cache
    ///        on the socket.
    /// @param[in] highWaterMark  The approximate max number of messages to 
    ///                           cache on the socket.  0 will set this to
    ///                           "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setReplierSendHighWaterMark(int highWaterMark);
    /// @result The send high water mark.  The default is 0.
    [[nodiscard]] int getReplierSendHighWaterMark() const noexcept;
    /// @}

    /// @name Capped Collection Options
    /// @{

    /// @brief The maximum number of packets that any can be held in
    ///        by any channel in the packet cache.
    /// @param[in] maxPackets  The max number of packets a channel can hold.
    ///                        For example, if the average packet duration is 
    ///                        two seconds, then maxPackets*2 would give the
    ///                        approximate temporal duration of this channel. 
    /// @throws std::invalid_argument if this is not positive.
    void setMaximumNumberOfPackets(const int maxPackets);
    /// @result The maximum number of packets that a channel in the packet cache
    ///         can hold. 
    [[nodiscard]] int getMaximumNumberOfPackets() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~ServiceOptions();
    /// @}

private:
    class ServiceOptionsImpl;
    std::unique_ptr<ServiceOptionsImpl> pImpl;
};
}
#endif

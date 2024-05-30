#ifndef URTS_SERVICES_SCALABLE_TRAVEL_TIMES_SERVICE_OPTIONS_HPP
#define URTS_SERVICES_SCALABLE_TRAVEL_TIMES_SERVICE_OPTIONS_HPP
#include <memory>
#include <chrono>
namespace UMPS::Authentication
{
 class ZAPOptions;
}
namespace URTS::Services::Scalable::TravelTimes
{
/// @class ServiceOptions "serviceOptions.hpp" "urts/services/scalable/travelTimes/serviceOptions.hpp"
/// @brief The options that define the backend travel time calculator
///        service.
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

    /// @name Replier Required Options
    /// @{
 
    /// @brief Sets the replier address to which to connect.
    /// @param[in] address  The address to which this backend service will
    ///                     connect.
    void setAddress(const std::string &address);
    /// @result The replier address to which to connect.
    /// @throws std::runtime_error if \c haveBackendAddress() is false.
    [[nodiscard]] std::string getAddress() const;
    /// @result True indicates the replier address was set.
    [[nodiscard]] bool haveAddress() const noexcept;
    /// @} 

    /// @name Optional Options
    /// @{

    /// @brief Sets the ZeroMQ Authentication Protocol options.
    /// @param[in] zapOptions  The ZAP options for the replier.
    void setZAPOptions(
        const UMPS::Authentication::ZAPOptions &zapOptions) noexcept;
    /// @result The ZAP options.
    [[nodiscard]] UMPS::Authentication::ZAPOptions getZAPOptions() const noexcept;

    /// @brief To receive replies we poll on a socket.  After this amount of
    ///        time has elapsed the process can proceed and handle other
    ///        essential activities like if the program has stopped.  If this
    ///        is too small then the thread will needlessly burn cycles on a
    ///        while loop but if its too big then thread will be unresponsive
    ///        on shut down.
    /// @param[in] timeOut  The time to wait for a request before the thread
    ///                     checks other things.
    /// @throws std::invalid_argument if this is negative.
    void setPollingTimeOut(const std::chrono::milliseconds &timeOut);
    /// @result The polling time out.
    [[nodiscard]] std::chrono::milliseconds getPollingTimeOut() const noexcept;
    /// @brief Influences the maximum number of request messages to cache
    ///        on the socket.
    /// @param[in] highWaterMark  The approximate max number of messages to 
    ///                           cache on the socket.  0 will set this to
    ///                           "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setReceiveHighWaterMark(int highWaterMark);
    /// @result The high water mark.  The default is 4096.
    [[nodiscard]] int getReceiveHighWaterMark() const noexcept;
    /// @brief Influences the maximum number of response messages to cache
    ///        on the socket.
    /// @param[in] highWaterMark  The approximate max number of messages to 
    ///                           cache on the socket.  0 will set this to
    ///                           "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setSendHighWaterMark(int highWaterMark);
    /// @result The send high water mark.  The default is 8192.
    [[nodiscard]] int getSendHighWaterMark() const noexcept;
    /// @}

    /// @name Travel Time Calculator Options
    /// @{

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

#ifndef UMPS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_REQUESTOR_OPTIONS_HPP
#define UMPS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_REQUESTOR_OPTIONS_HPP
#include <memory>
#include <chrono>
// Forward declarations
namespace UMPS::Authentication
{
class ZAPOptions;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class RequestorOptions "requestorOptions.hpp" "urts/services/scalable/associators/massociate/requestorOptions.hpp"
/// @brief Defines the options for the association requestor.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class RequestorOptions
{
public:
    /// @name Constructor
    /// @{

    /// @brief Constructor.
    RequestorOptions();
    /// @brief Copy constructor.
    /// @param[in] options  The options class from which to initialize
    ///                     this class.
    RequestorOptions(const RequestorOptions &options);
    /// @brief Move constructor.
    /// @param[in,out] options  The options class from which to initialize
    ///                         this class.  On exit, options's behavior
    ///                         is undefined.
    RequestorOptions(RequestorOptions &&options) noexcept;

    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] options  The options class to copy to this.
    /// @result A deep copy of options.
    RequestorOptions& operator=(const RequestorOptions &options);
    /// @brief Move assignment operator.
    /// @param[in,out] options  The options class whose memory will be moved
    ///                         to this.  On exit, options's behavior is
    ///                         undefined.
    /// @result The memory from options moved to this.
    RequestorOptions& operator=(RequestorOptions &&options) noexcept;

    /// @}

    /// @name Address 
    /// @{

    /// @brief Sets the address of the router to which requests will be
    ///        submitted and from which replies will be received.
    /// @param[in] address  The address of the router.
    /// @throws std::invalid_argument if address is blank.
    void setAddress(const std::string &address);
    /// @result The address to of the router.
    /// @throws std::runtime_error if \c haveAddress() is false.
    [[nodiscard]] std::string getAddress() const;
    /// @result True indicates that the address was set.
    [[nodiscard]] bool haveAddress() const noexcept;
    /// @}

    /// @name ZeroMQ Authentication Protocol
    /// @{

    /// @brief Sets the ZAP options.
    /// @param[in] options  The ZAP options which will define the socket's
    ///                     security protocol.
    void setZAPOptions(const UMPS::Authentication::ZAPOptions &options);
    /// @result The ZAP options.
    [[nodiscard]] UMPS::Authentication::ZAPOptions getZAPOptions() const noexcept;
    /// @}

    /// @name Time Out
    /// @{

    /// @param[in] timeOut  The amount of time to wait on a request before
    ///                     timing out.  A negative number will make this
    ///                     "infinite".
    void setReceiveTimeOut(const std::chrono::milliseconds &timeOut) noexcept;
    /// @result The receive timeout.  The default is 60 seconds.
    [[nodiscard]] std::chrono::milliseconds getReceiveTimeOut() const noexcept;
    /// @param[in] timeOut  The amount of time to wait to send a request before
    ///                     timing out.  A negative number will make this
    ///                     "infinite".
    void setSendTimeOut(const std::chrono::milliseconds &timeOut) noexcept;
    /// @result The send timeout.  The default is 0 seconds.
    [[nodiscard]] std::chrono::milliseconds getSendTimeOut() const noexcept;
    /// @}

    /// @name High Water Mark
    /// @{

    /// @param[in] highWaterMark  The approximate max number of response
    ///                           messages to cache on the socket.  0 will set
    ///                           this to "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setReceiveHighWaterMark(int highWaterMark);
    /// @result The high water mark.  The default is 8192.
    [[nodiscard]] int getReceiveHighWaterMark() const noexcept;

    /// @param[in] highWaterMark  The approximate max number of request
    ///                           messages to cache on the socket.  0 will set
    ///                           this to "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setSendHighWaterMark(int highWaterMark);
    /// @result The high water mark.  The default is 4096.
    [[nodiscard]] int getSendHighWaterMark() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~RequestorOptions();
    /// @}
private:
    class RequestorOptionsImpl;
    std::unique_ptr<RequestorOptionsImpl> pImpl;
};
}
#endif

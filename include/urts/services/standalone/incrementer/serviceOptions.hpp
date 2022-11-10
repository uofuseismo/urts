#ifndef URTS_SERVICES_STANDALONE_INCREMENTER_SERVICE_OPTIONS_HPP
#define URTS_SERVICES_STANDALONE_INCREMENTER_SERVICE_OPTIONS_HPP
#include <memory>
#include <chrono>
namespace UMPS::Authentication
{
 class ZAPOptions;
}
namespace URTS::Services::Standalone::Incrementer
{
/// @class ServiceOptions "serviceOptions.hpp" "urts/services/standalone/incrementer/serviceOptions.hpp"
/// @brief The options that define the backend incrementer service.
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
    ///                       incrementer service options to be parsed.
    /// @throws std::invalid_argument if the initialization file does not,
    ///         exist cannot be parsed, does not have the specified section,
    ///         or has incorrect information.
    void parseInitializationFile(const std::string &fileName,
                                 const std::string &section = "Incrementer");

    /// @name Required Connection Options
    /// @{

    /// @brief Sets the address to which to connect.
    /// @param[in] address  The address to which this backend service will
    ///                     connect.
    void setAddress(const std::string &address);
    /// @result The address to which to connect.
    /// @throws std::runtime_error if \c haveBackendAddress() is false.
    [[nodiscard]] std::string getAddress() const;
    /// @result True indicates the address was set.
    [[nodiscard]] bool haveAddress() const noexcept;
    /// @}

    /// @name Optional Connection Options
    /// @{

    /// @brief Sets the ZeroMQ Authentication Protocol options.
    /// @param[in] zapOptions  The ZAP options.
    void setZAPOptions(
        const UMPS::Authentication::ZAPOptions &zapOptions) noexcept;
    /// @result The ZAP options.
    [[nodiscard]]
    UMPS::Authentication::ZAPOptions getZAPOptions() const noexcept;

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
    /// @result The high water mark.  The default is 0.
    [[nodiscard]] int getReceiveHighWaterMark() const noexcept;
    /// @brief Influences the maximum number of response messages to cache
    ///        on the socket.
    /// @param[in] highWaterMark  The approximate max number of messages to 
    ///                           cache on the socket.  0 will set this to
    ///                           "infinite".
    /// @throws std::invalid_argument if highWaterMark is negative.
    void setSendHighWaterMark(int highWaterMark);
    /// @result The send high water mark.  The default is 0.
    [[nodiscard]] int getSendHighWaterMark() const noexcept;
    /// @}

    /// @name Optional Incrementer/Counter Options
    /// @{

    /// @brief Sets the sqlite3 file name.
    /// @param[in] fileName  The name of the sqlite3 file.
    void setSqlite3FileName(const std::string &fileName);
    /// @result The sqlite3 file containing the items to be incremented.
    [[nodiscard]] std::string getSqlite3FileName() const noexcept;

    /// @brief Allows the counter file to be deleted if it exists.
    /// @param[in] deleteIfExists  If true then the sqlite3 with counter
    ///                            information will be deleted and recreated
    ///                            on startup.
    void toggleDeleteSqlite3FileIfExists(bool deleteIfExists) noexcept;
    /// @result If true then the slqite3 file will deleted if it exists.
    [[nodiscard]] bool deleteSqlite3FileIfExists() const noexcept;

    /// @brief Sets the increment for all items.
    /// @param[in] increment  The amount by which to increment the counter.
    /// @throws std::invalid_argument if increment is not positive.
    void setIncrement(int increment);
    /// @result The increment.
    [[nodiscard]] int getIncrement() const noexcept;

    /// @brief The initial value for all items to be incremented.
    /// @param[in] initialValue   The initial value for all items to be
    ///                           incremented.
    /// @note The next value returned will be this plus the increment.
    void setInitialValue(int32_t initialValue) noexcept;
    /// @result The initial value of the incrementer.
    [[nodiscard]] int64_t getInitialValue() const noexcept;
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

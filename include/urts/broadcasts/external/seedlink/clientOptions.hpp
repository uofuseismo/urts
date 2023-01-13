#ifndef URTS_BROADCASTS_EXTERNAL_SEEDLINK_CLIENT_OPTIONS_HPP
#define URTS_BROADCASTS_EXTERNAL_SEEDLINK_CLIENT_OPTIONS_HPP
#include <memory>
namespace URTS::Broadcasts::External::SEEDLink
{
/// @class ClientOptions "clientOptions.hpp" "urts/broadcasts/external/seedlink/clientOptions.hpp"
/// @brief Defines the options used by the SEEDLink client.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ClientOptions
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ClientOptions();
    /// @brief Copy constructor.
    /// @param[in] options  The options from which to initialize this class.
    ClientOptions(const ClientOptions &options);
    /// @brief Move constructor.
    /// @param[in,out] options  The options from which to initialize this class.
    ///                         On exit, option's behavior is undefined.
    ClientOptions(ClientOptions &&options) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] options  The options class to copy to this.
    /// @result A deep copy of the options.
    ClientOptions& operator=(const ClientOptions &options);
    /// @brief Move assignment operator.
    /// @param[in,out] options  The options class whose memory will be moved
    ///                         to this.  On exit, option's behavior is
    ///                         undefined.
    /// @result The memory from options moved to this.
    ClientOptions& operator=(ClientOptions &&options) noexcept;
    /// @}

    /// @name Properties
    /// @{

    /// @param[in] address  The IP address of the SEEDLink server.
    void setAddress(const std::string &address);
    /// @result The IP address of the SEEDLink server.  By default this is
    ///         rtserve.iris.washington.edu
    [[nodiscard]] std::string getAddress() const noexcept;

    /// @brief Sets the port number of the SEEDLink server.
    /// @param[in] port  The port of the server.
    void setPort(uint16_t port) noexcept;
    /// @result The port number of the SEEDLink server.  By default this 18000.
    [[nodiscard]] uint16_t getPort() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~ClientOptions();
    /// @}
private:
    class ClientOptionsImpl;
    std::unique_ptr<ClientOptionsImpl> pImpl;
};
}
#endif

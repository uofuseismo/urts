#ifndef URTS_BROADCASTS_EXTERNAL_SEEDLINK_CLIENT_HPP
#define URTS_BROADCASTS_EXTERNAL_SEEDLINK_CLIENT_HPP
#include <memory>
#include <vector>
namespace UMPS::Logging
{
 class ILog;
}
namespace URTS::Broadcasts::External::SEEDLink
{
 class ClientOptions;
}
namespace URTS::Broadcasts::External::SEEDLink
{
/// @class Client "client.hpp" "urts/broadcasts/external/seedlink/client.hpp"
/// @brief Defines a SEEDLink client.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Client
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Client();
    /// @brief Constructor using the given logger.
    /// @param[in] logger  A pointer to the application's logger.
    explicit Client(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Move constructor.
    /// @param[in,out] client  Initializes the SEEDLink client from this class.
    ///                        On exit, client's behavior is undefined.
    Client(Client &&client) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Move assignment operator.
    /// @param[in,out] client  The client class whose memory will be moved
    ///                        to this.  On exit, client's behavior is
    ///                        undefined.
    /// @result The memory from client moved to this.
    Client& operator=(Client &&client) noexcept;
    /// @}

    /// @name Connection
    /// @{

    /// @brief Connects the client to the SEEDLink server.
    /// @throws std::runtime_error if the client cannot connect.
    void connect(const ClientOptions &options);
    /// @result True indicates that this class is connected to an
    ///         SEEDLink server.
    [[nodiscard]] bool isConnected() const noexcept;
    /// @}
    
    /// @name Reading
    /// @{

    /// @brief Reads from the SEEDLink server.
    /// @throws std::runtime_error if \c isConnected() is false.
    void read();
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Disconnects from the SEEDLink server.  Additionally, all
    ///        memory is released.
    void disconnect() noexcept;
    /// @brief Destructor.
    ~Client();
    /// @}

    Client& operator=(const Client &client) = delete;
    Client(const Client &client) = delete;
private:
    class ClientImpl;
    std::unique_ptr<ClientImpl> pImpl;
};
}
#endif

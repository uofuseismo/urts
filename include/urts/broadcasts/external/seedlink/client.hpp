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

    /// @brief Initializes the SEED Link client.
    /// @throws std::runtime_error if there are errors during initialization.
    void initialize(const ClientOptions &options);
    /// @result True indicates that this class is connected to an
    ///         SEEDLink server.
    [[nodiscard]] bool isInitialized() const noexcept;
    /// @}
    
    /// @name Usage
    /// @{

    /// @brief Reads from the SEEDLink server and populates the FIFO queue.
    /// @throws std::runtime_error if \c isInitialized() is false.
    void start();
    /// @brief Stops the SEEDLink client.
    void stop();
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

#ifndef URTS_DATABASE_CONNECTION_ICONNECTION_HPP
#define URTS_DATABASE_CONNECTION_ICONNECTION_HPP
#include <memory>
#include <cstdint>
#include "urts/database/connection/enums.hpp"
namespace URTS::Database::Connection
{
/// @class IConnection "connection.hpp" "urts/database/connection/connection.hpp"
/// @brief A base class for defining a database connection.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license. 
class IConnection
{
public:
    /// @brief Destructor.
    virtual ~IConnection() = default;
    /// @result Gets a handle to the session.
    [[nodiscard]] virtual std::uintptr_t getSession() const = 0;
    /// @result True indicates the connection is formed.
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;
    /// @result The database to which we are connected.
    [[nodiscard]] virtual DatabaseType getDatabaseType() const noexcept = 0;
};
}
#endif

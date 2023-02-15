#ifndef URTS_DATABASE_CONNECTION_ENUMS_HPP
#define URTS_DATABASE_CONNECTION_ENUMS_HPP
#include <memory>
#include <cstdint>
#include "urts/database/connection/enums.hpp"
namespace URTS::Database::Connection
{
/// @brief Defines a database type.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license. 
enum class DatabaseType
{
    PostgreSQL, /*!< PostgreSQL. */
    Sqlite3     /*!< SQLite3. */
};
}
#endif


#ifndef URTS_DATABASE_CONNECTION_POSTGRESQL_HPP
#define URTS_DATABASE_CONNECTION_POSTGRESQL_HPP
#include <memory>
#include "urts/database/connection/connection.hpp"
namespace URTS::Database::Connection
{
/// @name PostgreSQL "postgresql.hpp" "urts/database/connection/postgresql.hpp"
/// @brief Defines a PostgreSQL connection.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license. 
class PostgreSQL : public URTS::Database::Connection::IConnection
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    PostgreSQL();
    /// @brief Move constructor.
    /// @param[in,out] connection  The connection from which to initialize this
    ///                            class.
    PostgreSQL(PostgreSQL &&connection) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Move assignment.
    /// @param[in,out] connection  The connection whose memory will be moved to
    ///                            this.  On exit, connection's behavior is
    ///                            is undefined.
    /// @result The memory from the connection moved to this.
    PostgreSQL& operator=(PostgreSQL &&connection) noexcept;
    /// @}
    
    /// @name User Name
    /// @{
    /// @brief Sets the user name.
    /// @param[in] user  The user name.
    /// @throws std::invalid_argument if user is empty.
    void setUser(const std::string &user);
    /// @result The user name.
    /// @throws std::runtime_error if \c haveUser() is false.
    [[nodiscard]] std::string getUser() const;
    /// @result True indicates the user name was set.
    [[nodiscard]] bool haveUser() const noexcept;
    /// @}

    /// @name Password
    /// @{

    /// @brief Sets the user's password.
    /// @param[in] password  The user's password.
    /// @throws std::invalid_argument if password is empty.
    void setPassword(const std::string &password);
    /// @result The user's password.
    /// @throws std::runtime_error if \c havePassword() is false.
    [[nodiscard]] std::string getPassword() const;
    /// @result True indicates the user's password was set.
    [[nodiscard]] bool havePassword() const noexcept;
    /// @}

    /// @name Database Host Address
    /// @{

    /// @brief Sets the host's address.
    /// @param[in] address  The hosts's address e.g., machine.domain.com
    /// @throws std::invalid_argument if the address is empty.
    void setAddress(const std::string &address);
    /// @result The host address.
    /// @note By default this is 127.0.0.1
    [[nodiscard]] std::string getAddress() const noexcept;
    /// @}

    /// @name Database Name
    /// @{

    /// @brief Sets the name of the database.
    /// @throws std::invalid_argument if name is empty.
    void setDatabaseName(const std::string &name);
    /// @result The name of the database to which to connect.
    /// @throw std::runtime_error if \c haveDatabasename() is false.
    [[nodiscard]] std::string getDatabaseName() const;
    /// @result True indicates the database name was set.
    [[nodiscard]] bool haveDatabaseName() const noexcept;
    /// @}

    /// @name Port Number
    /// @{

    /// @brief Sets the port number.
    void setPort(int port);
    /// @result The port number.  By default this is 5432.
    [[nodiscard]] int getPort() const noexcept;
    /// @}

    /// @name Application Name
    /// @{

    /// @brief Sets the name of the application.
    /// @param[in] application  The name of the application.
    /// @throws std::invalid_argument if the application is empty.
    void setApplication(const std::string &application);
    /// @result The application name.
    /// @note By default this is qurts.
    [[nodiscard]] std::string getApplication() const noexcept;
    /// @}

    /// @name Driver
    /// @{

    /// @result The driver (e.g., postgresql).
    [[nodiscard]] static std::string getDriver() noexcept;
    /// @}

    /// @name Connect
    /// @{

    /// @result The connection string.
    [[nodiscard]] std::string getConnectionString() const;
    /// @brief Establishes a connection from the above resources.
    void connect() override;
    /// @result True indicates the connection was established.
    [[nodiscard]] bool isConnected() const noexcept override final;
    /// @}

    /// @result A shared pointer to the session.
    /// @throws std::runtime_error if \c isConnected() is false.
    [[nodiscard]] std::uintptr_t getSession() const override final;

    /// @result The database type.
    [[nodiscard]] DatabaseType getDatabaseType() const noexcept override final;

    /// @name Disconnect
    /// @{

    /// @brief Disconnects from teh database
    ///       (provided \c isConnected() is true).
    void disconnect() override;
    /// }

    /// @brief Loads the database connection information from an
    ///        initialization file.
    /// @param[in] fileName  The name of the initialization file.
    /// @param[in] section   The section of the initialization file
    ///                      with the requisite connection information.
    /// @throws std::invalid_argument if fileName cannot be found or
    ///         information cannot be found.
    /// @note The user, password and database name, if not specified, will
    ///       be extracted from the URTS_AQMS_RDONLY_USER,
    ///       URTS_AQMS_RDONLY_PASSWORD, and URTS_AQMS_RDONLY_DATABASE_NAME
    ///       environment variables, respectively.
    void parseInitializationFile(const std::string &fileName,
                                 const std::string &section = "AQMSPostgreSQL");

    /// @name Destructors
    /// @{

    /// @brief Destructor.
    ~PostgreSQL();
    /// @}

    PostgreSQL(const PostgreSQL &pg) = delete;
    PostgreSQL& operator=(const PostgreSQL &pg) = delete;
private:
    class PostgreSQLImpl;
    std::unique_ptr<PostgreSQLImpl> pImpl;
};
}
#endif

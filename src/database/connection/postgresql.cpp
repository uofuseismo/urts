#include <iostream>
#include <string>
#include <cstring>
#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <filesystem>
#include "urts/database/connection/postgresql.hpp"

using namespace URTS::Database::Connection;

class PostgreSQL::PostgreSQLImpl
{
public:
    PostgreSQLImpl()
    {
        mSessionPtr = &mSession;
        const char *userPtr = std::getenv("URTS_AQMS_RDONLY_USER");
        if (userPtr != nullptr)
        {
            if (std::strlen(userPtr) > 0)
            {
                mUser = std::string{userPtr};
            }
        }
        const char *passwordPtr = std::getenv("URTS_AQMS_RDONLY_PASSWORD");
        if (passwordPtr != nullptr)
        {
            if (std::strlen(passwordPtr) > 0)
            {
                mPassword = std::string{passwordPtr};
            }
        }
        const char *databasePtr = std::getenv("URTS_AQMS_DATABASE_NAME");
        if (databasePtr != nullptr)
        {
            if (std::strlen(databasePtr) > 0)
            {
                mDatabaseName = std::string {databasePtr};
            }
        }
std::cout << "yo " << mUser << " " << mPassword << " " << mDatabaseName<< std::endl;
    }
    soci::session mSession;
    void *mSessionPtr{nullptr};
    std::string mConnectionString;
    std::string mUser;
    std::string mPassword;
    std::string mDatabaseName;
    std::string mAddress{"127.0.0.1"};
    std::string mApplication{"urts"};
    int mPort{5432};
};

/// C'tor
PostgreSQL::PostgreSQL() :
    pImpl(std::make_unique<PostgreSQLImpl> ())
{
}

/// Move c'tor
PostgreSQL::PostgreSQL(PostgreSQL &&pg) noexcept
{
    *this = std::move(pg);
}

/// Move assignment
PostgreSQL& PostgreSQL::operator=(PostgreSQL &&pg) noexcept
{
    if (&pg == this){return *this;}
    pImpl = std::move(pg.pImpl);
    return *this;
}

/// Destructor
PostgreSQL::~PostgreSQL() = default; 

/// User
void PostgreSQL::setUser(const std::string &user)
{
    if (user.empty())
    {
        throw std::invalid_argument("user is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mUser = user;
}

std::string PostgreSQL::getUser() const
{
    if (!haveUser()){throw std::runtime_error("User not set");}
    return pImpl->mUser;
}

bool PostgreSQL::haveUser() const noexcept
{
    return !pImpl->mUser.empty();
}

/// Password
void PostgreSQL::setPassword(const std::string &password)
{
    if (password.empty())
    {
        throw std::invalid_argument("Password is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mPassword = password;
}

std::string PostgreSQL::getPassword() const
{
    if (!havePassword()){throw std::runtime_error("Password not set");}
    return pImpl->mPassword;
}

bool PostgreSQL::havePassword() const noexcept
{
    return !pImpl->mPassword.empty();
}

/// Address
void PostgreSQL::setAddress(const std::string &address)
{
    if (address.empty())
    {
        throw std::invalid_argument("Address is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mAddress = address;
}

std::string PostgreSQL::getAddress() const noexcept
{
    return pImpl->mAddress;
}

/// DB name
void PostgreSQL::setDatabaseName(const std::string &name)
{
    if (name.empty())
    {
        throw std::invalid_argument("Name is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mDatabaseName = name;
}

std::string PostgreSQL::getDatabaseName() const
{
    if (!haveDatabaseName()){throw std::runtime_error("Database name not set");}
    return pImpl->mDatabaseName;
}

bool PostgreSQL::haveDatabaseName() const noexcept
{
    return !pImpl->mDatabaseName.empty();
}

/// Port
void PostgreSQL::setPort(const int port)
{
    if (port < 0){throw std::invalid_argument("Port cannot be negative");}
    pImpl->mConnectionString.clear();
    pImpl->mPort = port;
}

int PostgreSQL::getPort() const noexcept
{
    return pImpl->mPort;
}

/// Application
void PostgreSQL::setApplication(const std::string &application)
{
    if (application.empty())
    {
        throw std::invalid_argument("Application is empty");
    }
    pImpl->mConnectionString.clear();
    pImpl->mApplication = application;
}

std::string PostgreSQL::getApplication() const noexcept
{
    return pImpl->mApplication;
}

/// Drivername
std::string PostgreSQL::getDriver() noexcept
{
   return "postgresql";
}

/// Generate a connection string
std::string PostgreSQL::getConnectionString() const
{
    if (!pImpl->mConnectionString.empty()){return pImpl->mConnectionString;}
    if (!haveUser()){throw std::runtime_error("User not set");}
    if (!havePassword()){throw std::runtime_error("Password not set");}
    auto user = getUser();
    auto password = getPassword();
    auto address = getAddress();
    auto cPort = std::to_string(getPort()); 
    auto dbname = getDatabaseName();
    auto appName = getApplication();
    pImpl->mConnectionString = //getDriver()
                             + "postgresql://" + user
                             + ":" + password
                             + "@" + address
                             + ":" + cPort
                             + "/" + dbname
                             + "?connect_timeout=10"
                             + "&application_name=" + appName;
    return pImpl->mConnectionString;
}

/// Connect
void PostgreSQL::connect()
{
    auto connectionString = getConnectionString(); // Throws
    if (pImpl->mSession.is_connected()){pImpl->mSession.close();}
    try
    {
        pImpl->mSession.open(soci::postgresql, connectionString);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to connect to postgresql with error:\n"
                               + std::string{e.what()});
    }
}

bool PostgreSQL::isConnected() const noexcept
{
    return pImpl->mSession.is_connected();
}

std::uintptr_t PostgreSQL::getSession() const
{
    return reinterpret_cast<std::uintptr_t> (pImpl->mSessionPtr);
}

DatabaseType PostgreSQL::getDatabaseType() const noexcept
{
    return DatabaseType::PostgreSQL;
}

/// Load the connection information
void PostgreSQL::parseInitializationFile(const std::string &fileName,
                                         const std::string &section)
{
    if (!std::filesystem::exists(fileName))
    {
        throw std::runtime_error("Initialization file: " + fileName
                               + " does not exist");
    }
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(fileName, propertyTree);
    // Load the variables with defaults 
    auto user = std::string(std::getenv("URTS_AQMS_RDONLY_USER"));
    user = propertyTree.get<std::string> (section + ".user", user);
    auto password = std::string(std::getenv("URTS_AQMS_RDONLY_PASSWORD"));
    password = propertyTree.get<std::string> (section + ".password", password);
    int port = getPort();
    port = propertyTree.get<int> (section + ".port", getPort());
    auto address = propertyTree.get<std::string> (section + ".address",
                                                  getAddress());
    auto databaseName
        = propertyTree.get<std::string> (section + ".databaseName");
    auto applicationName
        = propertyTree.get<std::string> (section + ".application",
                                          getApplication()); 
    // Set information
    setUser(user);
    setPassword(password);
    setPort(port);
    setDatabaseName(databaseName); 
    setAddress(address);
    setApplication(applicationName);
}

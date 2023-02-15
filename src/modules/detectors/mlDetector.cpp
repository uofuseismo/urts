#include <iostream>
#include <thread>
#include "urts/database/aqms/channelDataTablePoller.hpp"
#include "urts/database/aqms/channelDataTable.hpp"
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/postgresql.hpp"

struct ProgramOptions
{
public:
    
};

class NominalSamplingRate
{
public:
    
};

int main(int argc, char *argv[])
{
    auto pg = std::make_shared<URTS::Database::Connection::PostgreSQL> ();
    pg->setAddress("aqmsppt");
    pg->setDatabaseName("archdbt"); 
    try
    {
        pg->connect();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    auto connectionHandle
        = std::static_pointer_cast<URTS::Database::Connection::IConnection> (pg);
    URTS::Database::AQMS::ChannelDataTablePoller channelDataPoller;
    std::chrono::seconds pollerInterval{2};
    channelDataPoller.initialize(connectionHandle, URTS::Database::AQMS::ChannelDataTablePoller::QueryMode::Current, pollerInterval);
    channelDataPoller.start();
    std::this_thread::sleep_for(std::chrono::seconds {10});
    channelDataPoller.stop();
    return EXIT_SUCCESS;
}

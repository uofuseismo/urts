#include <iostream>
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
    URTS::Database::Connection::PostgreSQL pg;
    pg.setDatabaseName("aqmsrtt"); 
    pg.setPort(5432);
    try
    {
        pg.connect();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

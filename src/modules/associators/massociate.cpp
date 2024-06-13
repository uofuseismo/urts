#include <iostream>
#include <filesystem>
#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/logging/dailyFile.hpp> 
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/modules/processManager.hpp>
#include "urts/services/scalable/associators/massociate/associationRequest.hpp"
#include "urts/services/scalable/associators/massociate/associationResponse.hpp"
#include "urts/services/scalable/associators/massociate/requestor.hpp"
#include "urts/services/scalable/associators/massociate/requestorOptions.hpp"
#include "programOptions.hpp"

#define MODULE_NAME "mAssociator"

/// @result The logger for this application.
std::shared_ptr<UMPS::Logging::ILog>
createLogger(const std::string &moduleName = MODULE_NAME,
             const std::filesystem::path logFileDirectory = "/var/log/urts",
             const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
             const int hour = 0, 
             const int minute = 0) 
{
    auto logFileName = moduleName + ".log";
    auto fullLogFileName = logFileDirectory / logFileName;
    auto logger = std::make_shared<UMPS::Logging::DailyFile> ();  
    logger->initialize(moduleName,
                       fullLogFileName,
                       verbosity,
                       hour, minute);
    logger->info("Starting logging for " + moduleName);
    return logger;
}

/// @brief Parses the command line options.
[[nodiscard]] std::string parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The mAssociator drives the associators that turn picks into events.
Example usage:
    mAssociator --ini=mAssociator.ini
Allowed options)""");
    desc.add_options()
        ("help", "Produces this help message")
        ("ini",  boost::program_options::value<std::string> (),
                 "Defines the initialization file for this executable");
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm); 
    boost::program_options::notify(vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return iniFile;
    }
    if (vm.count("ini"))
    {
        iniFile = vm["ini"].as<std::string>();
        if (!std::filesystem::exists(iniFile))
        {
            throw std::runtime_error("Initialization file: " + iniFile
                                   + " does not exist");
        }
    }
    else
    {
        throw std::runtime_error("Initialization file was not set");
    }
    return iniFile;
}

int main(int argc, char *argv[])
{
    // Get the ini file from the command line
    std::string iniFile;
    try
    {
        iniFile = ::parseCommandLineOptions(argc, argv);
        if (iniFile.empty()){return EXIT_SUCCESS;}
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

/*
    // Create the logger
    constexpr int hour = 0; 
    constexpr int minute = 0; 
    auto logger = createLogger(programOptions.mModuleName,
                               programOptions.mLogFileDirectory,
                               programOptions.mVerbosity,
                               hour, minute);
    // Create the contexts (this is pretty low one should be fine)
    auto generalContext = std::make_shared<UMPS::Messaging::Context> (1);
    // Initialize the various processes
    logger->info("Initializing processes...");
    UMPS::Modules::ProcessManager processManager(logger);
*/
    return EXIT_SUCCESS;
}

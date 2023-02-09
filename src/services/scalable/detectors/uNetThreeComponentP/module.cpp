#include <iostream>
#include <filesystem>
#include <string>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/services/connectionInformation/requestorOptions.hpp>
#include <umps/services/connectionInformation/requestor.hpp>
#include <umps/services/connectionInformation/details.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/serviceOptions.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/service.hpp"
#include "private/isEmpty.hpp"

namespace UAuth = UMPS::Authentication;
namespace UCI = UMPS::Services::ConnectionInformation;
namespace UNet = URTS::Services::Scalable::Detectors::UNetThreeComponentP;

#define MODULE_NAME "UNetThreeComponentP"

/// @result Gets the command line input options as a string.
[[nodiscard]] std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   help       Displays this message.
)"""};
    return commands;
}

/// Parse the command line options
[[nodiscard]] std::pair<std::string, int>
    parseCommandLineOptions(int argc, char *argv[]);

/// @result The logger for this application.
std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName = MODULE_NAME,
                 const std::filesystem::path logFileDirectory = "/var/log/urts",
                 const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
                 const int instance = 0,
                 const int hour = 0,
                 const int minute = 0)
{
    auto logFileName = moduleName + "_" + std::to_string(instance) + ".log";
    auto fullLogFileName = logFileDirectory / logFileName;
    auto logger = std::make_shared<UMPS::Logging::DailyFile> (); 
    logger->initialize(moduleName,
                       fullLogFileName,
                       verbosity,
                       hour, minute);
    logger->info("Starting logging for " + moduleName);
    return logger;
}

/// @brief Defines the module options.
struct ProgramOptions
{
    /// Constructor with instance defaulting to 0
    ProgramOptions() :
       ProgramOptions(0)
    {
    }
    /// Constructor with given instance 
    explicit ProgramOptions(const int instance)
    {
       if (instance < 0)
       {
           throw std::invalid_argument("Instance must be positive");
       } 
       mInstance = instance;
    }
    /// @brief Load the module options from an initialization file.
    void parseInitializationFile(const std::string &iniFile)
    {
        boost::property_tree::ptree propertyTree;
        boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);
        //----------------------------- General ------------------------------//
        // Module name
        mModuleName
            = propertyTree.get<std::string> ("General.moduleName",
                                             mModuleName);
        if (mModuleName.empty())
        {
            throw std::runtime_error("Module name not defined");
        }
        // Verbosity
        mVerbosity = static_cast<UMPS::Logging::Level>
                     (propertyTree.get<int> ("General.verbose",
                                             static_cast<int> (mVerbosity)));
        // Log file directory
        mLogFileDirectory
            = propertyTree.get<std::string> ("General.logFileDirectory",
                                             mLogFileDirectory.string());
        if (!mLogFileDirectory.empty() &&
            !std::filesystem::exists(mLogFileDirectory))
        {
            std::cout << "Creating log file directory: "
                      << mLogFileDirectory << std::endl;
            if (!std::filesystem::create_directories(mLogFileDirectory))
            {
                throw std::runtime_error("Failed to make log directory");
            }
        }
        //-----------------------ML Model Options-----------------------------//
        auto weightsFile
            = propertyTree.get<std::string> ("UNetThreeComponentP", "");
        mServiceOptions.setModelWeightsFile(weightsFile);

        UNet::ServiceOptions::Device device = UNet::ServiceOptions::Device::CPU;
        auto deviceString
           = propertyTree.get<std::string> ("UNetThreeComponentP", "cpu");
        std::transform(deviceString.begin(), deviceString.end(),
                       deviceString.begin(), ::toupper);
        if (deviceString == std::string {"GPU"})
        {
            device = UNet::ServiceOptions::Device::GPU;
        }
        mServiceOptions.setDevice(device);
        //----------------Backend Service Connection Information--------------//
        mServiceName
             = propertyTree.get<std::string>
               ("UNetThreeComponentP.proxyServiceName", mServiceName);
    
        auto backendAddress
             = propertyTree.get<std::string>
               ("UNetThreeComponentP.proxyServiceAddress", "");
        if (!::isEmpty(backendAddress))
        {
            mServiceOptions.setAddress(backendAddress);
        }
        if (::isEmpty(mServiceName) && ::isEmpty(backendAddress))
        {
            throw std::runtime_error("Service backend address indeterminable");
        }
        mServiceOptions.setReceiveHighWaterMark(
            propertyTree.get<int> (
                "UNetThreeComponentP.proxyServiceReceiveHighWaterMark",
                mServiceOptions.getReceiveHighWaterMark())
        );
        mServiceOptions.setSendHighWaterMark(
            propertyTree.get<int> (
                "UNetThreeComponentP.proxyServiceSendHighWaterMark",
                mServiceOptions.getSendHighWaterMark())
        );
        auto pollingTimeOut 
            = static_cast<int> (mServiceOptions.getPollingTimeOut().count()
              );
        pollingTimeOut = propertyTree.get<int> (
              "PacketCache.proxyServicePollingTimeOut", pollingTimeOut);
        mServiceOptions.setPollingTimeOut(
            std::chrono::milliseconds {pollingTimeOut} );
    }
//public:
    UNet::ServiceOptions mServiceOptions;
    std::string mServiceName{"UNetThreeComponentPService"};
    std::string mHeartbeatBroadcastName{"Heartbeat"};
    std::string mModuleName{MODULE_NAME};
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    UAuth::ZAPOptions mZAPOptions;
    std::chrono::seconds heartBeatInterval{30};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    int mInstance{0};
};

int main(int argc, char *argv[])
{
    // Get the ini file from the command line
    std::string iniFile;
    int instance = 0;
    try
    {
        auto arguments = ::parseCommandLineOptions(argc, argv);
        iniFile = arguments.first;
        instance = arguments.second;
        if (iniFile.empty()){return EXIT_SUCCESS;}
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    // Parse the initialization file
    ProgramOptions programOptions(instance);
    try 
    {
        programOptions.parseInitializationFile(iniFile);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    // Create the logger
    constexpr int hour = 0;
    constexpr int minute = 0;
    auto logger = createLogger(programOptions.mModuleName,
                               programOptions.mLogFileDirectory,
                               programOptions.mVerbosity,
                               programOptions.mInstance,
                               hour, minute);
    // Create a context
    auto context = std::make_shared<UMPS::Messaging::Context> (1);
    // Initialize the various processes
    logger->info("Initializing processes...");
    UMPS::Modules::ProcessManager processManager(logger);

    return EXIT_SUCCESS;
}


///--------------------------------------------------------------------------///
///                            Utility Functions                             ///
///--------------------------------------------------------------------------///
/// Read the program options from the command line
std::pair<std::string, int> parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The uNetThreeComponentPService allows a detector module to apply the UUSS 
P-wave detector to the three-component data.  This is a scalable service so it
is likely that the user will have multiple instances running simultaneously so
as to satisfy the overall application's inference needs.  Example usage:
    uNetThreeComponentPService --ini=uNetThreeComponentPService.ini --instance=0
Allowed options)""");
    desc.add_options()
        ("help", "Produces this help message")
        ("ini",  boost::program_options::value<std::string> (),
                 "Defines the initialization file for this executable")
        ("instance",
         boost::program_options::value<uint16_t> ()->default_value(0),
         "Defines the module instance");
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);
    int instance = 0;
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return std::pair{iniFile, instance};
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
    if (vm.count("instance"))
    {
        instance = static_cast<int> (vm["instance"].as<uint16_t> ());
        if (instance < 0)
        {
            throw std::invalid_argument("Instance must be positive");
        }
    }
    return std::pair {iniFile, instance};
}


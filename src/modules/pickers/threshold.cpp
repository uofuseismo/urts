#include <iostream>
#include <string>
#include <map>
#include <chrono>
#ifndef NDEBUG
#include <cassert>
#endif
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/services/connectionInformation/requestorOptions.hpp>
#include <umps/services/connectionInformation/requestor.hpp>
#include <umps/services/connectionInformation/details.hpp>
#include <umps/services/connectionInformation/socketDetails/proxy.hpp>
#include <umps/services/connectionInformation/socketDetails/router.hpp>
#include <umps/services/connectionInformation/socketDetails/xSubscriber.hpp>
#include <umps/services/connectionInformation/socketDetails/xPublisher.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcess.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcessOptions.hpp>
#include "urts/broadcasts/internal/pick.hpp"
#include "urts/broadcasts/internal/probabilityPacket.hpp"
#include "urts/services/standalone/incrementer.hpp"
#include "thresholdDetectorOptions.hpp"
#include "thresholdDetector.hpp"
#include "triggerWindow.hpp"
#include "private/isEmpty.hpp"

#define MODULE_NAME "ThresholdPicker"

namespace
{

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
    //auto logger = std::make_shared<UMPS::Logging::StandardOut> (verbosity);
    auto logger = std::make_shared<UMPS::Logging::DailyFile> ();  
    logger->initialize(moduleName,
                       fullLogFileName,
                       verbosity,
                       hour, minute);
    logger->info("Starting logging for " + moduleName);
    return logger;
}


struct ProgramOptions
{
public:
    ProgramOptions()
    {
        mPThresholdDetectorOptions.setOnThreshold(0.8);
        mPThresholdDetectorOptions.setOffThreshold(0.72);
        mPThresholdDetectorOptions.setMinimumGapSize(5);

        mSThresholdDetectorOptions.setOnThreshold(0.85);
        mSThresholdDetectorOptions.setOffThreshold(0.72);
        mSThresholdDetectorOptions.setMinimumGapSize(5);
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
        //-------------------------- Detector Options ------------------------//
        mProbabilityPacketBroadcastName
            = propertyTree.get<std::string> (
                 "ThresholdPicker.probabilityPacketBroadcastName",
                 mProbabilityPacketBroadcastName);
        mProbabilityPacketBroadcastAddress
            = propertyTree.get<std::string> (
                 "ThresholdPicker.probabilityPacketBroadcastAddress",
                 mProbabilityPacketBroadcastAddress);
        if (::isEmpty(mProbabilityPacketBroadcastName) &&
            ::isEmpty(mProbabilityPacketBroadcastAddress))
        {
            throw std::runtime_error("Probability broadcast indeterminable");
        }

        mPickBroadcastName
            = propertyTree.get<std::string> (
                "ThresholdPicker.pickBroadcastName",
                mPickBroadcastName);
        mPickBroadcastAddress
            = propertyTree.get<std::string> (
                "ThresholdPicker.pickBroadcastAddress",
                mPickBroadcastAddress);
        if (::isEmpty(mPickBroadcastName) &&
            ::isEmpty(mPickBroadcastAddress))
        {
            throw std::runtime_error("Pick broadcast indeterminable");
        }

        mIncrementerServiceName
            = propertyTree.get<std::string> (
                "ThresholdPicker.incrementerServiceName",
                mIncrementerServiceName);
        mIncrementerServiceAddress
            = propertyTree.get<std::string> (
                "ThresholdPicker.pickBroadcastAddress",
                mIncrementerServiceAddress);
        if (::isEmpty(mIncrementerServiceName) &&
            ::isEmpty(mIncrementerServiceAddress))
        {
            throw std::runtime_error("Incrementer service indeterminable");
        }

        // P on/off thresholds
        auto onThreshold
            = propertyTree.get<double> ("ThresholdDetector.pOnThreshold",
                                   mPThresholdDetectorOptions.getOnThreshold());
        mPThresholdDetectorOptions.setOnThreshold(onThreshold);
        auto offThreshold
            = propertyTree.get<double> ("ThresholdDetector.pOffThreshold",
                                   mPThresholdDetectorOptions.getOnThreshold());
        mPThresholdDetectorOptions.setOffThreshold(offThreshold);
        auto gapSize
            = propertyTree.get<int> ("ThresholdDetector.pMinimumGapSize",
                               mPThresholdDetectorOptions.getMinimumGapSize());
        gapSize = std::max(0, gapSize);
        mPThresholdDetectorOptions.setMinimumGapSize(gapSize);

        // S on/off thresholds
        onThreshold
            = propertyTree.get<double> ("ThresholdDetector.sOnThreshold",
                                   mSThresholdDetectorOptions.getOnThreshold());
        mSThresholdDetectorOptions.setOnThreshold(onThreshold);
        offThreshold
            = propertyTree.get<double> ("ThresholdDetector.sOffThreshold",
                                   mSThresholdDetectorOptions.getOnThreshold());
        mSThresholdDetectorOptions.setOffThreshold(offThreshold);
        gapSize
            = propertyTree.get<int> ("ThresholdDetector.sMinimumGapSize",
                               mSThresholdDetectorOptions.getMinimumGapSize());
        gapSize = std::max(0, gapSize);
        mSThresholdDetectorOptions.setMinimumGapSize(gapSize);

        // Increment time out
        auto requestTimeOut
            = propertyTree.get<int>
                ("ThresholdDetector.incrementRequestTimeOut",
                 mIncrementRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mIncrementRequestReceiveTimeOut
            = std::chrono::milliseconds {requestTimeOut};

    }
    URTS::Broadcasts::Internal::ProbabilityPacket::SubscriberOptions
        mProbabilityPacketSubscriberOptions;
    URTS::Broadcasts::Internal::Pick::PublisherOptions mPickPublisherOptions;
    URTS::Services::Standalone::Incrementer::RequestorOptions
        mIncrementerRequestorOptions;
    URTS::Modules::Pickers::ThresholdDetectorOptions mPThresholdDetectorOptions;
    URTS::Modules::Pickers::ThresholdDetectorOptions mSThresholdDetectorOptions;
    std::string mModuleName{MODULE_NAME};
    std::string mProbabilityPacketBroadcastName{"ProbabilityPacket"};
    std::string mProbabilityPacketBroadcastAddress;
    std::string mPickBroadcastName{"Pick"};
    std::string mPickBroadcastAddress;
    std::string mIncrementerServiceName{"Incrementer"};
    std::string mIncrementerServiceAddress;
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::milliseconds mIncrementRequestReceiveTimeOut{1000}; // 1 s
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
};

/*
namespace
{
[[nodiscard]]
std::string toName(
    const URTS::Broadcasts::Internal::DataPacket::DataPacket &dataPacket)
{
    std::string result;
    if (dataPacket.haveNetwork())
    {
        result = result + dataPacket.getNetwork();
    }
    if (dataPacket.haveStation())
    {
        if (!result.empty())
        {
            if (result.back() != '.'){result = result + ".";}
        }
        result = result + dataPacket.getStation();
    }
    if (dataPacket.haveChannel())
    {
        if (!result.empty())
        {   
            if (result.back() != '.'){result = result + ".";}
        }
        result = result + dataPacket.getChannel();
    }
    if (dataPacket.haveLocationCode())
    {
        if (!result.empty())
        {
            if (result.back() != '.'){result = result + ".";}
        }
        result = result + dataPacket.getLocationCode();
    }
    return result;
}
}

class ThresholdPickersMap
{
public:
    // Constructor 
    ThresholdPickerMap(const std::string &name,
                       ThresholdDetectorOptions &options) :
        mName(name)
    {
        mDetector.initialize(options);
    }
    // 
    [[nodiscard]] std::string getName() const
    {
        return mname;
    }
    ThresholdDetector mDetector;
    std::string mName;
};

bool operator<(const ThresholdPickersMap &lhs,
               const ThresholdPickersMap &rhs)
{
    return lhs.getName() < rhs.getName();
}

class Threshold
{
public:
    // Applies the threshold detector
    void update(const URTS::Broadcasts::Internal::DataPacket::DataPacket
                &dataPacket)
    {
        auto name = ::toName(dataPacket);
        auto it = mPickers.find(name);
        if (it == mPickers.end())
        {
        }
        it = mPickers.find(name);
#ifndef NDEBUG
        assert(it != mPickers.end());
#endif
         
        mDetector.update(dataPacket);
    }
    [[nodiscard]] std::string getName() const
    {   
        return mname;
    }
    void processProbabilityBroadcast()
    {
        // Read the packets from the queue
        while (keepRunning())
        {
            auto message = subscriber->get();
            if (message != nullptr)
            {
                 
            }
        }
    }
    std::map<std::string, ThresholdDetector> mPickers;
};
*/

/// @brief Parses the command line options.
[[nodiscard]] std::string parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The mlDetector drives the UNet detectors and output P and S probability
signals to a probability broadcast.  Example usage:
    mlDetector --ini=mlDetector.ini
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
    // Parse the initialization file
    ProgramOptions programOptions;
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
    auto logger = ::createLogger(programOptions.mModuleName,
                                 programOptions.mLogFileDirectory,
                                 programOptions.mVerbosity,
                                 hour, minute);

    // Create the contexts
    auto context = std::make_shared<UMPS::Messaging::Context> (1);
    // Initialize the various processes
    logger->info("Initializing processes...");
    UMPS::Modules::ProcessManager processManager(logger);
    try
    {
        namespace UCI = UMPS::Services::ConnectionInformation;
        // Connect to the operator
        logger->debug("Connecting to uOperator...");
        const std::string operatorSection{"uOperator"};
        auto uOperator = UCI::createRequestor(iniFile, operatorSection,
                                              context, logger);
        auto zapOptions = uOperator->getZAPOptions();

        // Create a heartbeat
        logger->debug("Creating heartbeat process...");
        namespace UHeartbeat = UMPS::ProxyBroadcasts::Heartbeat;
        auto heartbeat = UHeartbeat::createHeartbeatProcess(*uOperator, iniFile,
                                                            "Heartbeat",
                                                            context,
                                                            logger);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        logger->error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

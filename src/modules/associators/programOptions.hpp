#ifndef URTS_MODULES_ASSOCIATORS_MASSOCIATOR_PROGRAM_OPTIONS_HPP
#define URTS_MODULES_ASSOCIATORS_MASSOCIATOR_PROGRAM_OPTIONS_HPP
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "urts/services/standalone/incrementer.hpp"
#include "urts/broadcasts/internal/origin.hpp"
#include "urts/broadcasts/internal/pick.hpp"
//#include "urts/services/associator/massociate.hpp"
#include "private/isEmpty.hpp"
namespace
{
struct ProgramOptions
{
public:
    ProgramOptions() = default;
    ProgramOptions& operator=(const ProgramOptions &) = default;
    ProgramOptions(const ProgramOptions &options)
    {
        *this = options;
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
/*
        //---------------------------- Database ------------------------------//
        const auto readOnlyUser = std::getenv("URTS_AQMS_RDONLY_USER");
        const auto readOnlyPassword = std::getenv("URTS_AQMS_RDONLY_PASSWORD");
        if (readOnlyUser != nullptr)
        {
            mDatabaseReadOnlyUser = std::string {readOnlyUser};
        }
        if (readOnlyPassword != nullptr)
        {
            mDatabaseReadOnlyPassword = std::string {readOnlyPassword};
        }
        mDatabaseReadOnlyUser
            = propertyTree.get<std::string> (
                 "MLDetector.databaseReadOnlyUser",
                 mDatabaseReadOnlyUser);
        if (mDatabaseReadOnlyUser.empty())
        {
            throw std::runtime_error("Database read-only user not set");
        }
        mDatabaseReadOnlyPassword
            = propertyTree.get<std::string> (
                "MLDetector.databaseReadOnlyPassword",
                mDatabaseReadOnlyPassword);
        if (mDatabaseReadOnlyPassword.empty())
        {
            throw std::runtime_error("Database read-only password not set");
        }
        mDatabasePort
            = propertyTree.get<int> ("MLDetector.databasePort", mDatabasePort);
        mDatabaseAddress
            = propertyTree.get<std::string> ("MLDetector.databaseAddress");
        if (mDatabaseAddress.empty())
        {
            throw std::runtime_error("Database address not set");
        }
        mDatabaseName
            = propertyTree.get<std::string> ("MLDetector.databaseName");
        if (mDatabaseName.empty())
        {
            throw std::runtime_error("Database name not set");
        }
        auto databasePollerInterval
            = propertyTree.get<int> ("MLDetector.databasePollerInterval",
                                     mDatabasePollerInterval.count());
        if (databasePollerInterval < 1)
        {
            throw std::runtime_error("Database poll interval must be positive");
        }
        mDatabasePollerInterval = std::chrono::seconds {databasePollerInterval};
 */
        //------------------------- Associator Options -----------------------//
        std::string section{"MAssociator"};

        auto region = propertyTree.get<std::string> (section + ".region");
        if (region == "utah" || region == "Utah")
        {
            mIsUtah = true;
        }
        else if (region == "YNP" || region == "ynp" || region == "Yellowstone")
        {
            mIsUtah = false;
        }
        else
        {
            throw std::invalid_argument("Unhandled region: " + region);
        }

        mPickBroadcastName
            = propertyTree.get<std::string> (
                 section + ".pickBroadcastName",
                 mPickBroadcastName);
        mPickBroadcastAddress
            = propertyTree.get<std::string> (
                 section + ".pickBroadcastAddress",
                 mPickBroadcastAddress);
        if (::isEmpty(mPickBroadcastName) &&
            ::isEmpty(mPickBroadcastAddress))
        {
            throw std::runtime_error("Pick broadcast indeterminable");
        }

        mOriginBroadcastName
            = propertyTree.get<std::string> (
                 section + ".originBroadcastName",
                 mOriginBroadcastName);
        mOriginBroadcastAddress
            = propertyTree.get<std::string> (
                 section + ".originBroadcastAddress",
                 mOriginBroadcastAddress);
        if (::isEmpty(mOriginBroadcastName) &&
            ::isEmpty(mOriginBroadcastAddress))
        {
            throw std::runtime_error("Origin broadcast indeterminable");
        }

        mAssociatorServiceName
            = propertyTree.get<std::string> (
                section + ".associatorServiceName",
                mAssociatorServiceName);
        mAssociatorServiceAddress
            = propertyTree.get<std::string> (
                section + ".associatorServiceAddress",
                mAssociatorServiceAddress);
        // Request time outs
        auto requestTimeOut
            = propertyTree.get<int> (section + ".associatorRequestTimeOut",
                                     mAssociatorRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mAssociatorRequestReceiveTimeOut
            = std::chrono::seconds {requestTimeOut};

        // Incrementer
        mIncrementerServiceName
            = propertyTree.get<std::string> (
                section + ".incrementerServiceName",
                mIncrementerServiceName);
        mIncrementerServiceAddress
            = propertyTree.get<std::string> (
                section + ".incrementerServiceAddress",
                mIncrementerServiceAddress);
        if (::isEmpty(mIncrementerServiceName) &&
            ::isEmpty(mIncrementerServiceAddress))
        {
            throw std::runtime_error("Incrementer service indeterminable");
        }
        // Increment time out
        requestTimeOut
            = propertyTree.get<int>
                (section + ".incrementRequestTimeOut",
                 mIncrementRequestReceiveTimeOut.count());
        if (requestTimeOut < 0) 
        {
            throw std::invalid_argument(
                "Increment request cannot indefinitely block");
        }
        mIncrementRequestReceiveTimeOut
            = std::chrono::milliseconds {requestTimeOut};


/*
        // Signal latency
        auto maximumSignalLatency
            = propertyTree.get<int> ("MLDetector.maximumSignalLatency",
                                     mMaximumSignalLatency.count());
        if (maximumSignalLatency <= 0)
        {
            throw std::runtime_error("Maximum signal latency must be positive");
        }
        mMaximumSignalLatency = std::chrono::seconds {maximumSignalLatency};
        // Figure out which algorithms are running
        mRunP3CDetector = propertyTree.get<bool> ("MLDetector.runP3CDetector",
                                                  mRunP3CDetector);
        mRunS3CDetector = propertyTree.get<bool> ("MLDetector.runS3CDetector",
                                                  mRunS3CDetector);
        mRunP1CDetector = propertyTree.get<bool> ("MLDetector.runP1CDetector",
                                                  mRunP1CDetector);
        if (!mRunP3CDetector && !mRunS3CDetector && !mRunP1CDetector)
        {
            throw std::runtime_error("No detectors to run!");
        }
        if (mRunP3CDetector && ::isEmpty(mP3CDetectorServiceName) &&
            ::isEmpty(mP3CDetectorServiceAddress))
        {
            throw std::runtime_error("P 3C service address unknowable");
        }
        if (mRunS3CDetector && ::isEmpty(mS3CDetectorServiceName) &&
            ::isEmpty(mS3CDetectorServiceAddress))
        {
            throw std::runtime_error("S 3C service address unknowable");
        }
        if (mRunP1CDetector && ::isEmpty(mP1CDetectorServiceName) &&
            ::isEmpty(mP1CDetectorServiceAddress))
        {
            throw std::runtime_error("P 1C service address unknowable");
        }
        // Gap tolerance 
        mGapTolerance = propertyTree.get<int> ("MLDetector.gapTolerance",
                                               mGapTolerance);
        // Figure out the valid networks
        std::string activeNetworks;
        for (const auto &activeNetwork : mActiveNetworks)
        {
            if (!activeNetworks.empty()){activeNetworks = activeNetworks + ",";}
            activeNetworks = activeNetworks + activeNetwork;
        }
        activeNetworks
            = propertyTree.get<std::string> ("MLDetector.activeNetworks",
                                             activeNetworks);
        std::vector<std::string> split;
        boost::split(split, activeNetworks, boost::is_any_of(",;\t"));
        mActiveNetworks.clear();
        for (const auto &network : split)
        {
            if (network.empty()){continue;}
            if (mActiveNetworks.find(network) == mActiveNetworks.end())
            {
                mActiveNetworks.insert(network);
            }
        }
        // Wait percentage for inference
        mDataQueryWaitPercentage
            = propertyTree.get<double> ("MLDetector.dataQueryWaitPercentage",
                                        mDataQueryWaitPercentage);
        if (mDataQueryWaitPercentage <= 0 || mDataQueryWaitPercentage >= 100)
        {
            throw std::invalid_argument("Wait pct must be in range (0,100)");
        }
        // Figure out the valid sampling rates
        std::string validSamplingRates;
        for (const auto &validSamplingRate : mValidSamplingRates)
        {
            if (!validSamplingRates.empty())
            {
                validSamplingRates = validSamplingRates + ",";
            }
            validSamplingRates = validSamplingRates
                               + std::to_string(validSamplingRate);
        }
        validSamplingRates
            = propertyTree.get<std::string> ("MLDetector.validSamplingRates",
                                             validSamplingRates);
        boost::split(split, validSamplingRates, boost::is_any_of(",;\t"));
        mValidSamplingRates.clear();
        for (const auto &sSamplingRate : split)
        {
            double samplingRate;
            try
            {
                samplingRate = std::stod(sSamplingRate);
            }
            catch (...)
            {
                continue;
            }
            if (samplingRate > 0 &&
                mValidSamplingRates.find(samplingRate) ==
                mValidSamplingRates.end())
            {
                mValidSamplingRates.insert(samplingRate);
            }
        }
*/
    }
    std::string mModuleName{"MAssociator"};
    std::string mAssociatorServiceName{"MAssociator"};
    std::string mAssociatorServiceAddress;
    std::string mPickBroadcastName{"Pick"};
    std::string mPickBroadcastAddress;
    std::string mOriginBroadcastName{"PreliminaryOrigin"};
    std::string mOriginBroadcastAddress;
    std::string mIncrementerServiceName{"Incrementer"};
    std::string mIncrementerServiceAddress;

    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    //std::chrono::seconds mDatabasePollerInterval{3600};
    std::chrono::seconds mAssociationWindowDuration{180};
    std::chrono::seconds mPickLatency{60};
    std::chrono::seconds mMaximumMoveout{45};
    std::chrono::seconds mAssociatorRequestReceiveTimeOut{60};
    std::chrono::milliseconds mIncrementRequestReceiveTimeOut{1000}; // 1 s
    std::set<std::string> mActiveNetworks;
    URTS::Services::Scalable::Associators::MAssociate::RequestorOptions
         mAssociatorRequestorOptions;
    URTS::Broadcasts::Internal::Pick::SubscriberOptions mPickSubscriberOptions;
    URTS::Broadcasts::Internal::Origin::PublisherOptions
         mOriginPublisherOptions;
    URTS::Services::Standalone::Incrementer::RequestorOptions
         mIncrementerRequestorOptions;
    double mDataQueryWaitPercentage{30};
    int mDatabasePort{5432};
    int mOriginHighWaterMark{0}; // Infinite
    //int mThreads{1};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    bool mIsUtah{true};
};
}
#endif

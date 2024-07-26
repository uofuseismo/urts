#ifndef URTS_MODULES_DETECTORS_PROGRAM_OPTIONS_HPP
#define URTS_MODULES_DETECTORS_PROGRAM_OPTIONS_HPP
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "urts/broadcasts/internal/dataPacket.hpp"
#include "urts/broadcasts/internal/probabilityPacket.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentS.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP.hpp"
#include "urts/services/scalable/packetCache.hpp"
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
        //--------------------------- Detector Options -----------------------//
        mThreads = propertyTree.get<int> ("MLDetector.nThreads", mThreads);
        if (mThreads < 1)
        {
            throw std::runtime_error("At least one thread required");
        }

        mProbabilityPacketBroadcastName
            = propertyTree.get<std::string> (
                 "MLDetector.probabilityPacketBroadcastName",
                 mProbabilityPacketBroadcastName);
        mProbabilityPacketBroadcastAddress
            = propertyTree.get<std::string> (
                 "MLDetector.probabilityPacketBroadcastAddress",
                 mProbabilityPacketBroadcastAddress);
        if (::isEmpty(mProbabilityPacketBroadcastName) &&
            ::isEmpty(mProbabilityPacketBroadcastAddress))
        {
            throw std::runtime_error("Probability broadcast indeterminable");
        }
        mP3CDetectorServiceName
            = propertyTree.get<std::string> (
                "MLDetector.pThreeComponentDetectorServiceName",
                mP3CDetectorServiceName);
        mP3CDetectorServiceAddress
            = propertyTree.get<std::string> (
                "MLDetector.pThreeComponentDetectorServiceAddress",
                mP3CDetectorServiceAddress);
        mS3CDetectorServiceName
            = propertyTree.get<std::string> (
                "MLDetector.sThreeComponentDetectorServiceName",
                mS3CDetectorServiceName);
        mS3CDetectorServiceAddress
            = propertyTree.get<std::string> (
                "MLDetector.sThreeComponentDetectorServiceAddress",
                mS3CDetectorServiceAddress);
        mP1CDetectorServiceName
            = propertyTree.get<std::string> (
                "MLDetector.pOneComponentDetectorServiceName",
                mP1CDetectorServiceName);
        mP1CDetectorServiceAddress
            = propertyTree.get<std::string> (
                "MLDetector.pOneComponentDetectorServiceAddress",
                mP1CDetectorServiceAddress);
        mPacketCacheServiceName
            = propertyTree.get<std::string> (
                "MLDetector.packetCacheServiceName",
                mPacketCacheServiceName);
        mPacketCacheServiceAddress
            = propertyTree.get<std::string> (
                "MLDetector.packetCacheServiceAddress",
                mPacketCacheServiceAddress);
        // Request time outs
        auto requestTimeOut
            = propertyTree.get<int> ("MLDetector.inferenceRequestTimeOut",
                                     mInferenceRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mInferenceRequestReceiveTimeOut
            = std::chrono::milliseconds {requestTimeOut};

        requestTimeOut
            = propertyTree.get<int> ("MLDetector.packetCacheRequestTimeOut",
                                     mDataRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mDataRequestReceiveTimeOut = std::chrono::milliseconds {requestTimeOut};
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
    }
    std::string mModuleName{"MLDetector"};
    std::string mPacketCacheServiceName{"RawDataPackets"};
    std::string mPacketCacheServiceAddress;
    std::string mProbabilityPacketBroadcastName{"ProbabilityPacket"};
    std::string mProbabilityPacketBroadcastAddress;
    std::string mP3CDetectorServiceName{"PDetector3C"};
    std::string mP3CDetectorServiceAddress;
    std::string mS3CDetectorServiceName{"SDetector3C"};
    std::string mS3CDetectorServiceAddress;
    std::string mP1CDetectorServiceName{"PDetector1C"};
    std::string mP1CDetectorServiceAddress;
    std::string mDatabaseAddress;
    std::string mDatabaseName;
    std::string mDatabaseReadOnlyUser;
    std::string mDatabaseReadOnlyPassword;
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::seconds mDatabasePollerInterval{3600};
    std::chrono::seconds mMaximumSignalLatency{180};
    std::chrono::milliseconds mInferenceRequestReceiveTimeOut{1000}; // 1 second
    std::chrono::milliseconds mDataRequestReceiveTimeOut{5000}; // 5 seconds
    std::set<std::string> mActiveNetworks;
    std::set<double> mValidSamplingRates;
    URTS::Services::Scalable::Detectors::UNetThreeComponentP::RequestorOptions
         mP3CDetectorRequestorOptions;
    URTS::Services::Scalable::Detectors::UNetThreeComponentS::RequestorOptions
         mS3CDetectorRequestorOptions;
    URTS::Services::Scalable::Detectors::UNetOneComponentP::RequestorOptions
         mP1CDetectorRequestorOptions;
    URTS::Services::Scalable::PacketCache::RequestorOptions
        mPacketCacheRequestorOptions;
    URTS::Broadcasts::Internal::ProbabilityPacket::PublisherOptions
        mProbabilityPacketPublisherOptions;
    double mDataQueryWaitPercentage{30};
    int mDatabasePort{5432};
    int mProbabilityPacketHighWaterMark{0}; // Infinite
    int mGapTolerance{5}; // In samples
    int mThreads{1};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    bool mRunP3CDetector{true};
    bool mRunP1CDetector{false};
    bool mRunS3CDetector{true};
};
}
#endif

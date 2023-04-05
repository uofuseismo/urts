#ifndef URTS_MODULES_PICKERS_MLPICKER_PROGRAM_OPTIONS_HPP
#define URTS_MODULES_PICKERS_MLPICKER_PROGRAM_OPTIONS_HPP
#include <iostream>
#include <string>
#include <chrono>
#include <cmath>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "urts/broadcasts/internal/pick.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS.hpp"
#include "urts/services/scalable/packetCache.hpp"
#include "private/isEmpty.hpp"
namespace
{
struct ProgramOptions
{
public:
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
                 "MLPicker.databaseReadOnlyUser",
                 mDatabaseReadOnlyUser);
        if (mDatabaseReadOnlyUser.empty())
        {
            throw std::runtime_error("Database read-only user not set");
        }
        mDatabaseReadOnlyPassword
            = propertyTree.get<std::string> (
                "MLPicker.databaseReadOnlyPassword",
                mDatabaseReadOnlyPassword);
        if (mDatabaseReadOnlyPassword.empty())
        {
            throw std::runtime_error("Database read-only password not set");
        }
        mDatabasePort
            = propertyTree.get<int> ("MLPicker.databasePort", mDatabasePort);
        mDatabaseAddress
            = propertyTree.get<std::string> ("MLPicker.databaseAddress");
        if (mDatabaseAddress.empty())
        {
            throw std::runtime_error("Database address not set");
        }
        mDatabaseName
            = propertyTree.get<std::string> ("MLPicker.databaseName");
        if (mDatabaseName.empty())
        {
            throw std::runtime_error("Database name not set");
        }
        auto databasePollerInterval
            = propertyTree.get<int> ("MLPicker.databasePollerInterval",
                                     mDatabasePollerInterval.count());
        if (databasePollerInterval < 1)
        {
            throw std::runtime_error("Database poll interval must be positive");
        }
        mDatabasePollerInterval = std::chrono::seconds {databasePollerInterval};
        //--------------------------- ML Model Options -----------------------//
        mThreads = propertyTree.get<int> ("MLPicker.nThreads", mThreads);
        if (mThreads < 1)
        {
            throw std::runtime_error("Number of threads must be positive");
        }

        mRunPPicker = propertyTree.get<bool> ("MLPicker.runPPicker",
                                              mRunPPicker);
        mRunSPicker = propertyTree.get<bool> ("MLPicker.runSPicker",
                                              mRunSPicker);
        mRunFirstMotionClassifier
            = propertyTree.get<bool> ("MLPicker.runFirstMotionClassifier",
                                      mRunFirstMotionClassifier);
        if (!mRunPPicker && !mRunSPicker && !mRunFirstMotionClassifier)
        {
            throw std::runtime_error("No models to run!");
        }
        if (mRunPPicker && ::isEmpty(mPPickerServiceName) &&
            ::isEmpty(mPPickerServiceAddress))
        {
            throw std::runtime_error("P picker service address unknowable");
        }
        if (mRunSPicker && ::isEmpty(mSPickerServiceName) &&
            ::isEmpty(mSPickerServiceAddress))
        {
            throw std::runtime_error("S picker service address unknowable");
        }
        if (mRunFirstMotionClassifier &&
            ::isEmpty(mFirstMotionClassifierServiceName) &&
            ::isEmpty(mFirstMotionClassifierServiceAddress))
        {
            throw std::runtime_error("FM classifer service address unknowable");
        }

        mInitialPickBroadcastName
            = propertyTree.get<std::string> (
                 "MLPicker.initialPickBroadcastName",
                 mInitialPickBroadcastName);
        mInitialPickBroadcastAddress
            = propertyTree.get<std::string> (
                 "MLPicker.initialPickBroadcastAddress",
                 mInitialPickBroadcastAddress);
        if (::isEmpty(mInitialPickBroadcastName) &&
            ::isEmpty(mInitialPickBroadcastAddress))
        {
            throw std::runtime_error("Initial pick broadcast indeterminable");
        }

        mRefinedPickBroadcastName
            = propertyTree.get<std::string> (
                "MLPicker.refinedPickBroadcastName",
                mRefinedPickBroadcastName);
        mRefinedPickBroadcastAddress
            = propertyTree.get<std::string> (
                "MLPicker.refinedPickBroadcastAddress",
                mRefinedPickBroadcastAddress);
        if (::isEmpty(mRefinedPickBroadcastName) &&
            ::isEmpty(mRefinedPickBroadcastAddress))
        {
            throw std::runtime_error("Refined pick broadcast indeterminable");
        }

        if (mRunPPicker)
        {
            mPPickerServiceName
                = propertyTree.get<std::string> (
                    "MLPicker.pPickerServiceName",
                    mPPickerServiceName);
            mPPickerServiceAddress
                = propertyTree.get<std::string> (
                    "MLPicker.pPickerServiceAddress",
                    mPPickerServiceAddress);
            if (::isEmpty(mPPickerServiceName) &&
                ::isEmpty(mPPickerServiceAddress))
            {
                throw std::runtime_error("P Picker service indeterminable");
            }
        }

        if (mRunSPicker)
        {
            mSPickerServiceName
                = propertyTree.get<std::string> (
                    "MLPicker.sPickerServiceName",
                    mSPickerServiceName);
            mSPickerServiceAddress
                = propertyTree.get<std::string> (
                    "MLPicker.sPickerServiceAddress",
                    mSPickerServiceAddress);
            if (::isEmpty(mSPickerServiceName) &&
                ::isEmpty(mSPickerServiceAddress))
            {
                throw std::runtime_error("S Picker service indeterminable");
            }
        }

        if (mRunFirstMotionClassifier)
        {
            mFirstMotionClassifierServiceName
                = propertyTree.get<std::string> (
                    "MLPicker.firstMotionClassifierServiceName",
                    mFirstMotionClassifierServiceName);
            mFirstMotionClassifierServiceAddress
                = propertyTree.get<std::string> (
                    "MLPicker.firstMotionClassifierServiceAddress",
                    mFirstMotionClassifierServiceAddress);
            if (::isEmpty(mFirstMotionClassifierServiceName) &&
                ::isEmpty(mFirstMotionClassifierServiceAddress))
            {
                throw std::runtime_error("First motion service indeterminable");
            }
        }

        mPacketCacheServiceName
            = propertyTree.get<std::string> (
                "MLPicker.packetCacheServiceName",
                mPacketCacheServiceName);
        mPacketCacheServiceAddress
            = propertyTree.get<std::string> (
                "MLPicker.packetCacheServiceAddress",
                mPacketCacheServiceAddress);
        if (::isEmpty(mPacketCacheServiceName) &&
            ::isEmpty(mPacketCacheServiceAddress))
        {
            throw std::runtime_error("Packet cache service indeterminable");
        }

        // Request time out
        auto requestTimeOut
            = propertyTree.get<int> ("MLDetector.inferenceRequestTimeOut",
                                     mInferenceRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mInferenceRequestReceiveTimeOut
            = std::chrono::milliseconds {requestTimeOut};

        // Increment time out
        requestTimeOut
            = propertyTree.get<int> ("MLDetector.incrementRequestTimeOut",
                                     mIncrementRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mIncrementRequestReceiveTimeOut
            = std::chrono::milliseconds {requestTimeOut};

        // 

        // Gap tolerance
        mGapTolerance = propertyTree.get<int> ("MLPicker.gapTolerance",
                                               mGapTolerance);

    }
    URTS::Broadcasts::Internal::Pick::PublisherOptions
         mRefinedPickPublisherOptions;
    URTS::Broadcasts::Internal::Pick::SubscriberOptions
         mInitialPickSubscriberOptions;
    URTS::Services::Scalable::PacketCache::RequestorOptions
        mPacketCacheRequestorOptions;
    URTS::Services::Scalable::Pickers::CNNOneComponentP::RequestorOptions
        mPPickerRequestorOptions;
    URTS::Services::Scalable::Pickers::CNNThreeComponentS::RequestorOptions
        mSPickerRequestorOptions;
    URTS::Services::Scalable::FirstMotionClassifiers::
        CNNOneComponentP::RequestorOptions
        mFirstMotionClassifierRequestorOptions;
    std::string mModuleName{"MLPicker"};
    std::string mRefinedPickBroadcastName{"RefinedPick"};
    std::string mRefinedPickBroadcastAddress;
    std::string mPacketCacheServiceName{"RawDataPackets"};
    std::string mPacketCacheServiceAddress;
    std::string mPPickerServiceName{"PPicker"};
    std::string mPPickerServiceAddress;
    std::string mSPickerServiceName{"SPicker"};
    std::string mSPickerServiceAddress;
    std::string mFirstMotionClassifierServiceName{"FirstMotionClassifier"};
    std::string mFirstMotionClassifierServiceAddress;
    std::string mInitialPickBroadcastName{"InitialPick"};
    std::string mInitialPickBroadcastAddress;
    std::string mDatabaseAddress;
    std::string mDatabaseName;
    std::string mDatabaseReadOnlyUser;
    std::string mDatabaseReadOnlyPassword;
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::milliseconds mInferenceRequestReceiveTimeOut{2000}; // 2 second
    std::chrono::milliseconds mDataRequestReceiveTimeOut{5000}; // 5 seconds
    std::chrono::milliseconds mIncrementRequestReceiveTimeOut{5000}; // 5 seconds
    std::chrono::seconds mDatabasePollerInterval{3600};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    int mDatabasePort{5432};
    int mGapTolerance{5};
    int mThreads{2};
    bool mRunPPicker{true};
    bool mRunSPicker{true};
    bool mRunFirstMotionClassifier{true};
};
}
#endif

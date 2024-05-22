#include <thread>
#include <array>
#include <mutex>
#include <cmath>
#include <string>
#include <cstring>
#include <queue>
#include <umps/logging/standardOut.hpp>
#include </usr/local/include/libmseed.h>
#include <libslink.h>
#include <slplatform.h>
#include "urts/broadcasts/external/seedlink/client.hpp"
#include "urts/broadcasts/external/seedlink/clientOptions.hpp"
#include "urts/broadcasts/external/seedlink/streamSelector.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"

using namespace URTS::Broadcasts::External::SEEDLink;
namespace UDataPacket = URTS::Broadcasts::Internal::DataPacket;

namespace
{
/// @brief Unpacks a miniSEED record.
[[nodiscard]]
UDataPacket::DataPacket
    miniSEEDToDataPacket(char *msRecord,
                         const int seedLinkRecordSize = 512)
{
    UDataPacket::DataPacket dataPacket;
    constexpr int8_t verbose{0};
    constexpr uint32_t flags{MSF_UNPACKDATA};
    MS3Record *miniSEEDRecord{nullptr};
    auto returnValue = msr3_parse(msRecord, seedLinkRecordSize,
                                  &miniSEEDRecord, flags,
                                  verbose);
    if (returnValue == 0)
    {
        // SNCL
        std::array<char, 64> networkWork;
        std::array<char, 64> stationWork;
        std::array<char, 64> channelWork;
        std::array<char, 64> locationWork;
        std::fill(networkWork.begin(),  networkWork.end(), '\0');
        std::fill(stationWork.begin(),  stationWork.end(), '\0');
        std::fill(channelWork.begin(),  channelWork.end(), '\0'); 
        std::fill(locationWork.begin(), locationWork.end(), '\0');
        returnValue = ms_sid2nslc(miniSEEDRecord->sid,
                                  networkWork.data(), stationWork.data(),
                                  locationWork.data(), channelWork.data());
        std::string network{networkWork.data()};
        std::string station{stationWork.data()};
        std::string channel{channelWork.data()};
        std::string location{locationWork.data()};
        if (locationWork[0] == '\0'){location = "--";}
        if (std::string {"  "} == location.substr(0, 2)){location = "--";}
        if (returnValue == 0)
        {
            dataPacket.setNetwork(network);
            dataPacket.setStation(station);
            dataPacket.setChannel(channel);
            dataPacket.setLocationCode(location);
        }
        else
        {
            msr3_free(&miniSEEDRecord);
            throw std::runtime_error("Failed to unpack SNCL");
        }
        // Sampling rate
        dataPacket.setSamplingRate(miniSEEDRecord->samprate);
        // Start time (convert from nanoseconds to microseconds)
        std::chrono::microseconds startTime
        {
            static_cast<int64_t> (std::round(miniSEEDRecord->starttime*1.e-3))
        };
        dataPacket.setStartTime(startTime);
        // Data
        auto nSamples = static_cast<int> (miniSEEDRecord->numsamples);
        if (nSamples > 0)
        {
            if (miniSEEDRecord->sampletype == 'i')
            {
                const auto data
                     = reinterpret_cast<const int *>
                       (miniSEEDRecord->datasamples);
                dataPacket.setData(nSamples, data);
            }
            else if (miniSEEDRecord->sampletype == 'f')
            {
                const auto data
                     = reinterpret_cast<const float *>
                       (miniSEEDRecord->datasamples);
                dataPacket.setData(nSamples, data);
            }
            else if (miniSEEDRecord->sampletype == 'd')
            {
                const auto data
                     = reinterpret_cast<const double *> 
                       (miniSEEDRecord->datasamples); 
                dataPacket.setData(nSamples, data);
            }
            else
            {
                msr3_free(&miniSEEDRecord);
                throw std::runtime_error("Unhandled sample type");
            }
        }
    }
    else
    {
        if (returnValue < 0)
        {
            msr3_free(&miniSEEDRecord);
            throw std::runtime_error("libmseed error detected");
        }
        msr3_free(&miniSEEDRecord);
        throw std::runtime_error(
             "Insufficient data.  Number of additional bytes estimated is "
            + std::to_string(returnValue));
    }
    // Cleanup
    msr3_free(&miniSEEDRecord);
    // Return
    return dataPacket;
}
}

class Client::ClientImpl
{
public:
    ClientImpl(std::shared_ptr<UMPS::Logging::ILog> logger = nullptr) :
        mLogger(logger)
    {
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> (); 
        }
    }
    /// Destructor
    ~ClientImpl()
    {
        stop();
        disconnect();
    }
    /// Terminate the SEED link client connection
    void disconnect()
    {
        if (mSEEDLinkConnection != nullptr)
        {
            if (mSEEDLinkConnection->link != -1)
            {
                mLogger->debug("Disconnecting SEEDLink...");
                sl_disconnect(mSEEDLinkConnection);
            }
            if (mUseStateFile)
            {
                mLogger->debug("Saving state prior to disconnect...");
                sl_savestate(mSEEDLinkConnection, mStateFile.c_str());
            }
            mLogger->debug("Freeing SEEDLink structure...");
            sl_freeslcd(mSEEDLinkConnection);
            mSEEDLinkConnection = nullptr;
        }
    }
    /// Sends a terminate command to the SEEDLink connection
    void terminate()
    {
        if (mSEEDLinkConnection != nullptr)
        {
            mLogger->debug("Issuing terminate command to poller");
            sl_terminate(mSEEDLinkConnection);
        }
    }
    /// Toggles this as running or not running
    void setRunning(const bool running)
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        // Terminate the session
        if (!running && mKeepRunning)
        {
            mLogger->debug("Issuing terminate command");
            terminate();
        }
        mKeepRunning = running;
    }
    /// Keep running?
    [[nodiscard]] bool keepRunning() const noexcept
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mKeepRunning;
    }
    /// Stops the service
    void stop()
    {
        setRunning(false); // Issues terminate command
        if (mSEEDLinkReaderThread.joinable()){mSEEDLinkReaderThread.join();}
    }
    /// Starts the service
    void start()
    {
        stop(); // Ensure module is stopped
        if (!mInitialized)
        {
            throw std::runtime_error("SEEDLink client not initialized");
        }
        setRunning(true);
        mLogger->debug("Starting the SEEDLink polling thread...");
        mSEEDLinkReaderThread = std::thread(&ClientImpl::scrapePackets,
                                            this);
    }
    /// Gets the latest packets from the SEEDLink server and puts them in the
    /// internal queue for shipping off to the data broadcast.
    void scrapePackets()
    {
        // Reset the queue
        clearQueue();
        // Recover state
        if (mUseStateFile)
        {
            if (!sl_recoverstate(mSEEDLinkConnection, mStateFile.c_str()))
            {
                 mLogger->warn("Failed to recover state");
            }
        }
        // Now start scraping
        SLpacket *seedLinkPacket{nullptr};
        int updateStateFile{1};
        while (keepRunning())
        {
            // Block until a packet is received.  Alternatively, an external
            // thread can terminate the broadcast.  In this case we will quit.
            auto returnValue = sl_collect(mSEEDLinkConnection, &seedLinkPacket);
            // Deal with packet
            if (returnValue == SLPACKET)
            {
	        auto packetType = sl_packettype(seedLinkPacket);
                // I really only care about data packets
                if (packetType == SLDATA)
                {
	            //auto sequenceNumber = sl_sequence(seedLinkPacket);
                    auto miniSEEDRecord
                        = reinterpret_cast<char *> (seedLinkPacket->msrecord);
                    try
                    {
                        auto packet = miniSEEDToDataPacket(miniSEEDRecord,
                                                           mSEEDRecordSize);
                        addPacket(std::move(packet));
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->error("Skipping packet.  Unpacking failed with: "
                                     + std::string(e.what()));
                    }
                    if (mUseStateFile)
                    {
                        if (updateStateFile > mStateFileUpdateInterval)
                        {
                            sl_savestate(mSEEDLinkConnection,
                                         mStateFile.c_str());
                            updateStateFile = 0;
                        }
                        updateStateFile = updateStateFile + 1;
                    }
                }
            }
            else if (returnValue == SLTERMINATE)
            {
                mLogger->debug("SEEDLink terminate request received");
                break;
            }
            else
            {
                mLogger->warn("Unhandled SEEDLink return value: "
                            + std::to_string(returnValue));
                continue;
            }
        }
        mLogger->debug("Thread leaving SEEDLink polling loop");
    }
    /// Update the queue
    void addPacket(UDataPacket::DataPacket &&dataPacket)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mDataPackets.size() > mMaximumQueueSize)
        {
            mDataPackets.pop();
        }
        mDataPackets.push(std::move(dataPacket));
    }
    /// Reset queue
    void clearQueue()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        std::queue<UDataPacket::DataPacket> ().swap(mDataPackets);
    }
    /// Get the latest batch of packets
    [[nodiscard]] std::vector<UDataPacket::DataPacket> getPackets() const
    {
        std::vector<UDataPacket::DataPacket> result;
        std::lock_guard<std::mutex> lock(mMutex);
        result.reserve(mDataPackets.size());
        while (!mDataPackets.empty())
        {
            result.push_back(std::move(mDataPackets.front()));
            mDataPackets.pop();
        }
        return result;
    } 
    /// @brief A container with the value at the front of the queue provided
    ///        that the queue is not empty.  If the queue is not empty then
    ///        then this value is removed from the front. 
    /// @result The value at the front of the queue or a nullptr if the queue
    ///         was empty. 
    [[nodiscard]] std::shared_ptr<UDataPacket::DataPacket> try_pop()
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        std::shared_ptr<UDataPacket::DataPacket> result;
        if (mDataPackets.empty())
        {
            result = nullptr;
            return result;
        }
        result
            = std::make_shared<UDataPacket::DataPacket> (mDataPackets.front());
        mDataPackets.pop();
        return result;
    }
    /// @result True indicates that the queue is empty.
    [[nodiscard]] bool empty() const
    {   
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mDataPackets.empty();
    }   
    /// @result The number of elements in the queue.
    [[nodiscard]] size_t size() const
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mDataPackets.size();
    }
//private:
    mutable std::mutex mMutex;
    std::thread mSEEDLinkReaderThread;
    SLCD *mSEEDLinkConnection{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr}; 
    ClientOptions mOptions;
    mutable std::queue<UDataPacket::DataPacket> mDataPackets;
    /*
    const std::array<std::string, 10> PacketType{ "Data",
                                                  "Detection",
                                                  "Calibration",
                                                  "Timing",
                                                  "Message",
                                                  "General",
                                                  "Request",
                                                  "Info",
                                                  "Info (terminated)",
                                                  "KeepAlive" };
    */
    std::string mStateFile;
    int mSEEDRecordSize{512};
    int mMaxPacketsRead{0};
    int mStateFileUpdateInterval{100};
    size_t mMaximumQueueSize{8192};
    bool mInitialized{false};
    bool mUseStateFile{false};
    bool mKeepRunning{false};
};
    
/// Constructor
Client::Client() :
    pImpl(std::make_unique<ClientImpl> ())
{
}

/// Constructor with logger
Client::Client(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ClientImpl> (logger))
{
}

/// Constructor
Client::~Client() = default;

/// Is initialized?
bool Client::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Connect
void Client::initialize(const ClientOptions &options)
{
    pImpl->disconnect(); // Hangup first
    pImpl->clearQueue(); // Release queue
    pImpl->mInitialized = false;
    // Create a new instance
    pImpl->mSEEDLinkConnection = sl_newslcd();
    // Set the connection string
    auto address = options.getAddress();
    auto port = options.getPort();
    auto seedLinkAddress = address +  ":" + std::to_string(port);
    pImpl->mLogger->debug("Connecting to SEEDLink server "
                        + seedLinkAddress + "...");
    pImpl->mSEEDLinkConnection->sladdr = strdup(seedLinkAddress.c_str());
    // Set the record size and state file
    pImpl->mSEEDRecordSize = options.getSEEDRecordSize();
    if (options.haveStateFile())
    { 
        pImpl->mStateFile = options.getStateFile();
        pImpl->mStateFileUpdateInterval = options.getStateFileUpdateInterval();
        pImpl->mUseStateFile = true;
    }
    // Queue size
    pImpl->mMaximumQueueSize
        = static_cast<size_t> (options.getMaximumInternalQueueSize());
    // 
    // TODO selectors (sl_addstream(pImpl->mSEEDLinkConnection, net, sta, streamselect, -1, NULL));
    constexpr int sequenceNumber{-1}; // Start at next data
    const char *timeStamp{nullptr};
    auto streamSelectors = pImpl->mOptions.getStreamSelectors();
    for (const auto &selector : streamSelectors)
    {
        try
        {
            auto network = selector.getNetwork();
            auto station = selector.getStation();
            auto streamSelector = selector.getSelector();
            pImpl->mLogger->info("Adding: "
                                + network + " "
                                + station + " "
                                + streamSelector);
            auto returnCode = sl_addstream(pImpl->mSEEDLinkConnection,
                                           network.c_str(),
                                           station.c_str(),
                                           streamSelector.c_str(),
                                           sequenceNumber,
                                           timeStamp);
            if (returnCode != 0)
            {
                throw std::runtime_error("Failed to add selector: " 
                                       + network + " " 
                                       + station + " "
                                       + streamSelector);
            }
        }
        catch (const std::exception &e)
        {
            pImpl->mLogger->warn("Could not add selector because "
                               + std::string {e.what()});
        }
    }
    // Configure uni-station mode if no streams were specified
    if (pImpl->mSEEDLinkConnection->streams == nullptr)
    {
        const char *selectors{nullptr};
        auto returnCode = sl_setuniparams(pImpl->mSEEDLinkConnection,
                                          selectors, sequenceNumber, timeStamp);
        if (returnCode != 0)
        {
            pImpl->mLogger->error("Could not set SEEDLink uni-station mode");
            throw std::runtime_error(
                "Failed to create a SEEDLink uni-station client");
        }
    }
    // Time out and reconnect delay
    auto networkTimeOut
        = static_cast<int> (options.getNetworkTimeOut().count());
    pImpl->mSEEDLinkConnection->netto = networkTimeOut;
    auto reconnectDelay
        = static_cast<int> (options.getNetworkReconnectDelay().count());
    pImpl->mSEEDLinkConnection->netdly = reconnectDelay;
    // Check this worked
    std::string slSite(256, '\0');
    std::string slServerID(256, '\0');
    auto returnCode = sl_ping(pImpl->mSEEDLinkConnection,
                              slServerID.data(),
                              slSite.data());
    if (returnCode != 0)
    {
        if (returnCode ==-1)
        {
            pImpl->mLogger->warn("Invalid ping response");
        }
        else
        {
            pImpl->mLogger->error("Could not connect to server");
            throw std::runtime_error("Failed to connected");
        }
    }
    // All-good
    pImpl->mOptions = options;
    pImpl->mInitialized = true;
}

/// Start the reader
void Client::start()
{
    if (!isInitialized())
    {
        throw std::runtime_error("SEED Link client not initialized");
    }
    pImpl->start();
}

/// Stop the reader
void Client::stop()
{
    pImpl->stop();
}

/// Gets the packets
std::vector<UDataPacket::DataPacket> Client::getPackets() const
{
    return pImpl->getPackets();
}

/// Empty?
bool Client::empty() const
{
    return pImpl->empty();
}

/// Size
size_t Client::size() const
{
    return pImpl->size();
}

std::shared_ptr<UDataPacket::DataPacket> Client::try_pop()
{
    return pImpl->try_pop();
}

#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <umps/logging/standardOut.hpp>
#include "urts/database/aqms/channelDataTablePoller.hpp"
#include "urts/database/aqms/channelDataTable.hpp"
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/connection.hpp"

using namespace URTS::Database::AQMS;

class ChannelDataTablePoller::ChannelDataTablePollerImpl
{
public:
    /// Constructor
    ChannelDataTablePollerImpl(
        std::shared_ptr<UMPS::Logging::ILog> logger) :
        mLogger(logger)
    {
        if (mLogger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        mChannelDataTable = std::make_unique<ChannelDataTable> (mLogger);
    }
    /// Destructor
    ~ChannelDataTablePollerImpl()
    {
        stop();
    }
    /// Stops the service
    void stop()
    {
        mLogger->debug("Stopping database polling service...");
        setKeepRunning(false);
        if (mPollerThread.joinable()){mPollerThread.join();}
    } 
    /// Starts the service
    void start()
    {
        stop();
        mLogger->debug("Starting database polling service...");
        setKeepRunning(true);
        mPollerThread = std::thread(&ChannelDataTablePollerImpl::poll,
                                    this); 
    }
    /// Poll/Update
    void poll()
    {
        mLogger->debug("Beginning database polling loop...");
        while (keepRunning())
        {
            // Update
            try
            {
                if (mQueryMode == QueryMode::Current)
                {
                    mLogger->debug("Querying current channel data...");
                    mChannelDataTable->queryCurrent();
                }
                else
                {
                    mLogger->debug("Querying all channel data...");
                    mChannelDataTable->queryAll();
                }
            }
            catch (const std::exception &e)
            {
                mLogger->warn("Poller detected query error: " + std::string {e.what()});
            }
            std::unique_lock<std::mutex> lock(mStopContext);
            mStopCondition.wait_for(lock,
                                    mPollingInterval,
                                    [this]
                                    {
                                        return !keepRunning();
                                    });
            lock.unlock();
        }
        mLogger->debug("Exited database polling loop");
/*
        while (keepRunning())
        {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto nowSeconds
                = std::chrono::duration_cast<std::chrono::seconds> (now);
            auto nextQueryTime = mLastQuery + mPollingInterval;
            if (nowSeconds > nextQueryTime)
            {
                mLogger->debug("Querying channel data...");
                if (mQueryMode == QueryMode::Current)
                {
                    mChannelDataTable->queryCurrent();
                }
                else
                {
                    mChannelDataTable->queryAll();
                }
                mLastQuery = nowSeconds;
            }
            std::this_thread::sleep_for (std::chrono::seconds(1));
        }
*/
    }
    /// Keep running?
    [[nodiscard]] bool keepRunning() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mKeepRunning;
    }
    /// Determines whether this should / should not keep running
    void setKeepRunning(const bool running)
    {
        {
            std::scoped_lock lock(mMutex);
            mKeepRunning = running;
        }
        mStopCondition.notify_one(); // Tell the poller that we're done
    }
    /// Sets the connection
    void setConnection(
        std::shared_ptr<URTS::Database::Connection::IConnection> &connection)
    {
        mChannelDataTable->setConnection(connection);
        mConnected = mChannelDataTable->isConnected();
    }
    bool isInitialized() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mConnected && mInitialized;
    }
    [[nodiscard]] std::vector<ChannelData>
        getChannelData(const std::string &network,
                       const std::string &station,
                       const std::string &channel,
                       const std::string &locationCode) const
    {
        std::scoped_lock lock(mMutex);
        return mChannelDataTable->getChannelData(network, station,
                                                 channel, locationCode);
    }
    [[nodiscard]] std::vector<ChannelData> getChannelData() const
    {
        std::scoped_lock lock(mMutex);
        return mChannelDataTable->getChannelData();
    }
///private:
    mutable std::mutex mMutex;
    std::mutex mStopContext;
    std::thread mPollerThread;
    std::condition_variable mStopCondition;
    std::unique_ptr<ChannelDataTable> mChannelDataTable{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::chrono::seconds mPollingInterval{3600};
    //std::chrono::seconds mLastQuery;
    QueryMode mQueryMode{QueryMode::Current};
    bool mKeepRunning{false};
    bool mConnected{false};
    bool mInitialized{false};
};

/// C'tor
ChannelDataTablePoller::ChannelDataTablePoller() :
    pImpl(std::make_unique<ChannelDataTablePollerImpl> (nullptr))
{
}

ChannelDataTablePoller::ChannelDataTablePoller(
    std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ChannelDataTablePollerImpl> (logger))
{
}

/// Destructor
ChannelDataTablePoller::~ChannelDataTablePoller() = default;

/// Set connection
void ChannelDataTablePoller::initialize(
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection,
    const QueryMode queryMode,
    const std::chrono::seconds &pollingInterval)
{
    if (!connection->isConnected())
    {
        throw std::invalid_argument("Session not connected");
    }
    if (pollingInterval.count() < 0)
    {
        throw std::invalid_argument("Refresh rate must be positive");
    }
    pImpl->mInitialized = false;
    stop();
    // Create the connection
    pImpl->setConnection(connection);
    pImpl->mPollingInterval = pollingInterval;
    pImpl->mQueryMode = queryMode;
    pImpl->mInitialized = true;
}

/// Initialized?
bool ChannelDataTablePoller::isInitialized() const noexcept
{
    return pImpl->isInitialized();
}

/// Stop the service
void ChannelDataTablePoller::stop()
{
    pImpl->stop();
}

/// Starts the service
void ChannelDataTablePoller::start()
{
    if (!isInitialized()){throw std::runtime_error("Not initialized");}
    pImpl->start();
}

/// Get result from query
std::vector<ChannelData> ChannelDataTablePoller::getChannelData() const
{
    return pImpl->getChannelData();
}


std::vector<ChannelData> ChannelDataTablePoller::getChannelData(
    const std::string &network,
    const std::string &station,
    const std::string &channel,
    const std::string &locationCode) const
{
    return pImpl->getChannelData(network, station, channel, locationCode);
}

/// Polling interval
std::chrono::seconds ChannelDataTablePoller::getPollingInterval() const noexcept
{
    return pImpl->mPollingInterval;
}

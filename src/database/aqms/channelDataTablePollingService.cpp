#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <umps/logging/standardOut.hpp>
#include "urts/database/aqms/channelDataTablePollingService.hpp"
#include "urts/database/aqms/channelDataTable.hpp"
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/connection.hpp"

using namespace URTS::Database::AQMS;

class ChannelDataTablePollingService::ChannelDataTablePollingServiceImpl
{
public:
    /// Constructor
    ChannelDataTablePollingServiceImpl(
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
    ~ChannelDataTablePollingServiceImpl()
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
        mPollerThread = std::thread(&ChannelDataTablePollingServiceImpl::poll,
                                    this); 
    }
    /// Poll/Update
    void poll()
    {
        mLogger->debug("Beginning database polling loop...");
        while (keepRunning())
        {
            std::unique_lock<std::mutex> lock(mStopContext);
            mStopCondition.wait_for(lock,
                                    mRefreshRate,
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
            auto nextQueryTime = mLastQuery + mRefreshRate;
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
    bool isConnected() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mConnected;
    }
    void setOptions(const std::chrono::seconds &refreshRate,
                    const QueryMode mode)
    {
        std::scoped_lock lock(mMutex);
        mRefreshRate = refreshRate;
        mQueryMode = mode;
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
    std::chrono::seconds mRefreshRate{3600};
    //std::chrono::seconds mLastQuery;
    QueryMode mQueryMode{QueryMode::Current};
    bool mKeepRunning{false};
    bool mConnected{false};
};

/// C'tor
ChannelDataTablePollingService::ChannelDataTablePollingService() :
    pImpl(std::make_unique<ChannelDataTablePollingServiceImpl> (nullptr))
{
}

ChannelDataTablePollingService::ChannelDataTablePollingService(
    std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ChannelDataTablePollingServiceImpl> (logger))
{
}

/// Destructor
ChannelDataTablePollingService::~ChannelDataTablePollingService() = default;

/// Set connection
void ChannelDataTablePollingService::setConnection(
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection)
{
    stop();
    pImpl->setConnection(connection); 
}

/// Connected?
bool ChannelDataTablePollingService::isConnected() const noexcept
{
    return pImpl->isConnected();
}

/// Stop the service
void ChannelDataTablePollingService::stop()
{
    pImpl->stop();
}

/// Starts the service
void ChannelDataTablePollingService::start(
    const std::chrono::seconds &refreshRate,
    const QueryMode mode)
{
    if (!isConnected()){throw std::runtime_error("Not connected");}
    if (refreshRate.count() < 0)
    {
        throw std::invalid_argument("Refresh rate must be positive");
    }
    pImpl->setOptions(refreshRate, mode);
    pImpl->start();
}

/// Get result from query
std::vector<ChannelData> ChannelDataTablePollingService::getChannelData(
    const std::string &network,
    const std::string &station,
    const std::string &channel,
    const std::string &locationCode) const
{
    return pImpl->getChannelData(network, station, channel, locationCode);
}


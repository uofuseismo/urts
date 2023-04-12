#include <string>
#include <set>
#include <map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <umps/logging/standardOut.hpp>
#include "urts/services/scalable/packetCache/cappedCollection.hpp"
#include "urts/services/scalable/packetCache/circularBuffer.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "utilities.hpp"
#include "stringMatch.hpp"

using namespace URTS::Services::Scalable::PacketCache;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;

/// Implementation
class CappedCollection::CappedCollectionImpl
{
public:
    /// C'tor
    CappedCollectionImpl() :
        mLogger(std::make_shared<UMPS::Logging::StandardOut> ())
    {
    }
    /// C'tor
    explicit CappedCollectionImpl(std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mLogger(logger)
    {
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
    }
    /// Destructor
    ~CappedCollectionImpl()
    {
        clear();
        mLogger = nullptr;
    }
    /// Reset class
    void clear() noexcept
    {
        std::scoped_lock lock(mMutex);
        mCircularBufferMap.clear();
        mMaxPackets = 0;
        mInitialized = false; 
    }
    /// Have sensor?
    [[nodiscard]] bool haveSensor(const std::string &name) const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mCircularBufferMap.contains(name);
    }
    /// Get all sensor names
    [[nodiscard]] std::unordered_set<std::string> getSensors() const noexcept
    {
        std::unordered_set<std::string> result;
        std::scoped_lock lock(mMutex);
        result.reserve(mCircularBufferMap.size());
        for (auto it = mCircularBufferMap.begin();
             it != mCircularBufferMap.end(); ++it)
        {
            result.insert(it->first);
        }
        return result;
    }
    /// Update (with move semantics for speed - i.e., no copies)
    void update(UDP::DataPacket &&packet)
    {
        auto name = makeName(packet);
        std::scoped_lock lock(mMutex);
        auto it = mCircularBufferMap.find(name);
        if (it == mCircularBufferMap.end())
        {
            mLogger->debug("Adding: " + name);
            CircularBuffer cbNew;
            cbNew.initialize(packet.getNetwork(),
                             packet.getStation(),
                             packet.getChannel(),
                             packet.getLocationCode(),
                             mMaxPackets);
            cbNew.addPacket(std::move(packet));
            mCircularBufferMap.insert(std::pair(name, cbNew));
        }
        else
        {
            if (mLogger->getLevel() >= UMPS::Logging::Level::Debug)
            {
                mLogger->debug("Updating: " + name);
            }
            it->second.addPacket(std::move(packet));
        } 
    }
    /// True indicates the channel is blacklisted 
    [[nodiscard]] bool isBlackListed(const UDP::DataPacket &packet)
    {
        bool isBlackListed = false;
        if (mBlackList.empty()){return isBlackListed;}
        if (packet.haveChannel())
        {
            auto channel = packet.getChannel();
            for (const auto &pattern : mBlackList)
            {
                if (::stringMatch(channel, pattern))
                {
                    isBlackListed = true;
                    break;
                }
            }
        }
        return isBlackListed;
    } 
    /// Get total number of packets
    [[nodiscard]] int getTotalNumberOfPackets() const noexcept
    {
        int nPackets = 0;
        std::scoped_lock lock(mMutex);
        for (auto it = mCircularBufferMap.begin();
             it != mCircularBufferMap.end(); ++it)
        {
            nPackets = nPackets + it->second.getNumberOfPackets();
        }
        return nPackets;
    }
    /// Query
    std::vector<UDP::DataPacket>
        getPackets(const std::string &name,
                   const std::chrono::microseconds &t0) const
    {
        std::scoped_lock lock(mMutex);
        auto it = mCircularBufferMap.find(name);
        if (it != mCircularBufferMap.end())
        {
            return it->second.getPackets(t0);
        }
        throw std::invalid_argument("Sensor: " + name
                                  + " not in collection");
    }
    /// Query
    std::vector<UDP::DataPacket>
        getPackets(const std::string &name,
                   const std::chrono::microseconds &t0,
                   const std::chrono::microseconds &t1) const 
    {
        std::scoped_lock lock(mMutex);
        auto it = mCircularBufferMap.find(name);
        if (it != mCircularBufferMap.end())
        {
            return it->second.getPackets(t0, t1);
        }
        throw std::invalid_argument("Sensor: " + name
                                  + " not in collection");
    }
    /// Get earliest start time
    std::chrono::microseconds
        getEarliestStartTime(const std::string &name) const
    {
        std::scoped_lock lock(mMutex);
        auto it = mCircularBufferMap.find(name);
        if (it != mCircularBufferMap.end())
        {   
            return it->second.getEarliestStartTime();
        }
        return std::chrono::microseconds{std::numeric_limits<int>::lowest()};
    }
///private:
    mutable std::mutex mMutex;
    std::map<std::string, CircularBuffer> mCircularBufferMap;
    std::set<std::string> mBlackList;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    int mMaxPackets{0};
    bool mInitialized{false};
};

/// C'tor
CappedCollection::CappedCollection() :
    pImpl(std::make_unique<CappedCollectionImpl> ())
{
}

/// C'tor
CappedCollection::CappedCollection(
    std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<CappedCollectionImpl> (logger))
{
}

/// Destructor
CappedCollection::~CappedCollection() = default;

/// Initialization
void CappedCollection::initialize(const int maxPackets,
                                  const std::set<std::string> &blackList)
{
    clear();
    if (maxPackets < 1)
    {
        throw std::invalid_argument("Max number of packets = "
                                  + std::to_string(maxPackets)
                                  + " must be positive");
    }
    pImpl->mBlackList = blackList;
    pImpl->mMaxPackets = maxPackets;
    pImpl->mInitialized = true;
}

/// Initialized?
bool CappedCollection::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Add a packet
void CappedCollection::addPacket(const UDP::DataPacket &packet)
{
    auto packetCopy = packet;
    addPacket(std::move(packetCopy));
}

/// Add packet with move
void CappedCollection::addPacket(UDP::DataPacket &&packet)
{
    if (pImpl->isBlackListed(packet)){return;}
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    if (!isValidPacket(packet))
    {
        throw std::invalid_argument("Packet is invalid");
    }
    pImpl->update(std::move(packet));
}

/// Reset the class
void CappedCollection::clear() noexcept
{
    pImpl->clear();
}

/// Have SNCL?
bool CappedCollection::haveSensor(
    const std::string &network, const std::string &station,
    const std::string &channel, const std::string &locationCode) const noexcept
{
    if (!isInitialized()){return false;}
    auto name = makeName(network, station, channel, locationCode);
    return haveSensor(name);
}

bool CappedCollection::haveSensor(const std::string &name) const noexcept
{
    if (!isInitialized()){return false;}
    return pImpl->haveSensor(name);
}

/// Get all the sensor names
std::unordered_set<std::string>
    CappedCollection::getSensorNames() const noexcept
{
    return pImpl->getSensors();
}

/// Get total number of packets
int CappedCollection::getTotalNumberOfPackets() const noexcept
{
    return pImpl->getTotalNumberOfPackets();
}

/// Earliest time
std::chrono::microseconds
    CappedCollection::getEarliestStartTime(const std::string &name) const
{
    auto t = pImpl->getEarliestStartTime(name);
    if (t == std::chrono::microseconds{std::numeric_limits<int>::lowest()})
    {
        throw std::runtime_error("Sensor " + name
                               + " does not exist in collection");
    }
    return t;
}


/// Get packets from t0 to now
std::vector<UDP::DataPacket>
    CappedCollection::getPackets(const std::string &name,
                                 const std::chrono::microseconds &t0) const
{
    if (!haveSensor(name))
    {
        throw std::runtime_error("Sensor " + name + " not in collection");
    }
    return pImpl->getPackets(name, t0); 
}

std::vector<UDP::DataPacket>
    CappedCollection::getPackets(const std::string &name,
                                    const double t0) const
{
    return getPackets(name, ::secondsToMicroSeconds(t0));
}

/// Get packets from t0 to t1 
std::vector<UDP::DataPacket>
    CappedCollection::getPackets(const std::string &name,
                                 const std::chrono::microseconds &t0,
                                 const std::chrono::microseconds &t1) const
{
    if (!haveSensor(name))
    {   
        throw std::runtime_error("Sensor " + name + " not in collection");
    }
    if (t1 <= t0)
    {
        throw std::invalid_argument("t0 = " + std::to_string(t0.count())
                                  + " must be less than t1 = "
                                  + std::to_string(t1.count()));
    }
    return pImpl->getPackets(name, t0, t1);
}

std::vector<UDP::DataPacket>
    CappedCollection::getPackets(const std::string &name,
                                 const double t0,
                                 const double t1) const
{
    return getPackets(name,
                      ::secondsToMicroSeconds(t0),
                      ::secondsToMicroSeconds(t1));
}

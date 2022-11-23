#include <iostream>
#include <cmath>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/circular_buffer.hpp>
#include "urts/services/scalable/packetCache/circularBuffer.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "private/isEmpty.hpp"
#include "utilities.hpp"

using namespace URTS::Services::Scalable::PacketCache;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;

#define NAN_TIME std::chrono::microseconds{std::numeric_limits<int64_t>::lowest()}

class CircularBuffer::CircularBufferImpl
{
public:
    void update(UDP::DataPacket &&packet)
    {
        auto t0 = packet.getStartTime(); 
        std::scoped_lock lock(mMutex);
        if (mCircularBuffer.capacity() == 0L)
        {
            throw std::runtime_error("Circular capacity is 0");
        }
        // Empty buffer case
        if (mCircularBuffer.empty())
        {
            mCircularBuffer.push_back(std::move(packet));
            return;
        }
        // Most common thing will be a new piece of data at end
        auto t1Buffer = mCircularBuffer.back().getStartTime();
        if (t0 > t1Buffer)
        {
            mCircularBuffer.push_back(std::move(packet));
            return;
        }
        // Now the joy of backfilling data begins.  Is the data too old?
        auto t0Buffer = mCircularBuffer.front().getStartTime();
        // Data expired and the buffer is full so skip it. 
        if (t0 < t0Buffer && mCircularBuffer.full()){return;}
        // The packet is not too old so it will go somewhere in the CB.
        // Find the packet whose start time is greater than or equal to this
        // packet's start value.
        auto it = std::upper_bound(mCircularBuffer.begin(),
                                   mCircularBuffer.end(), packet,
                                   [](const UDP::DataPacket &lhs,
                                      const UDP::DataPacket &rhs)
                                   {
                                      return lhs.getStartTime() <
                                             rhs.getStartTime();
                                   });
        // By this point we shouldn't be pointing to the end of the CB.
        if (it != mCircularBuffer.end())
        {
            // We have an exact match - ovewrite the old packet
            auto index = std::distance(mCircularBuffer.begin(), it);
            auto t0Neighbor = mCircularBuffer[index].getStartTime();
            if (t0Neighbor == t0)
            {
                mCircularBuffer[index] = std::move(packet);
                return; 
            }
            // Insert the element before its upper bounding element
            mCircularBuffer.rinsert(it, std::move(packet));
            // Debug code checks this is sorted
#ifndef NDEBUG
            assert(std::is_sorted(mCircularBuffer.begin(),
                                  mCircularBuffer.end(),
                                  [](const UDP::DataPacket &lhs,
                                     const UDP::DataPacket &rhs)
                                  {
                                     return lhs.getStartTime() <
                                            rhs.getStartTime();
                                  }));
#endif
            // This isn't beautiful but now we insert the packet at the end
            // and sort the (mostly sorted circular buffer).  Basically,
            // backfilling is an ugly process we want to do as little as
            // possible.
            //mCircularBuffer.push_back(packet);
            //mCircularBuffer.rinsert(it, packet);
            //std::sort(mCircularBuffer.begin(), mCircularBuffer.end(),
            //          [](const UDP::DataPacket &lhs,
            //             const UDP::DataPacket &rhs)
            //          {
            //             return lhs.getStartTime() < rhs.getStartTime();
            //          });
        }
    }
    [[nodiscard]] std::chrono::microseconds getEarliestStartTime() const
    {
        std::scoped_lock lock(mMutex);
        if (mCircularBuffer.empty())
        {
            return NAN_TIME;
        }
        const auto &packet = mCircularBuffer.front(); 
        return packet.getStartTime();
    }
    // Get all packets currently in buffer
    [[nodiscard]] std::vector<UDP::DataPacket> getAllPackets() const
    {
        std::vector<UDP::DataPacket> result;
        std::scoped_lock lock(mMutex);
        auto nPackets = mCircularBuffer.size();
        result.reserve(nPackets); 
        for (const auto &packet : mCircularBuffer)
        {
            result.push_back(packet);
        }
        return result;
    }
    // Perform query from now until whenever
    [[nodiscard]] std::vector<UDP::DataPacket>
        getPackets(const std::chrono::microseconds t0MuS,
                   const std::chrono::microseconds t1MuS) const
    {
        std::vector<UDP::DataPacket> result;
        std::scoped_lock lock(mMutex);
        if (mCircularBuffer.empty()){return result;}
        auto it0 = std::upper_bound(mCircularBuffer.begin(),
                                    mCircularBuffer.end(), t0MuS,
                                    [](const std::chrono::microseconds t0MuS,
                                       const UDP::DataPacket &rhs)
                                    {
                                       return t0MuS <= rhs.getStartTime();
                                    });
        if (it0 == mCircularBuffer.end()){return result;}
        // Attempt to move back one b/c of upper_bound works
        if (it0 != mCircularBuffer.begin() && it0->getStartTime() > t0MuS)
        {
            it0 = std::prev(it0, 1);
            // If the end time of the previous packet is before t0MuS
            // then restore the iterator as this packet is too old.
            if (it0->getEndTime() < t0MuS){it0 = std::next(it0);}
        }
        // For efficiency's sake when we query with
        auto it1 = mCircularBuffer.end();
        if (t1MuS < mCircularBuffer.back().getStartTime())
        {
            it1 = std::upper_bound(mCircularBuffer.begin(),
                                   mCircularBuffer.end(), t1MuS,
                                   [](const std::chrono::microseconds t1MuS,
                                      const UDP::DataPacket &rhs)
                                   {
                                      return t1MuS < rhs.getStartTime();
                                   });
        }
        // Just one packet
        if (it0 == it1)
        {
            result.push_back(*it0);
            return result;
        }
        // General copy
#ifndef NDEBUG
        // Don't want an infinite copy
        assert(std::distance(mCircularBuffer.begin(), it0) <
               std::distance(mCircularBuffer.begin(), it1));
#endif
        auto nPackets = static_cast<int> (std::distance(it0, it1));
        if (nPackets < 1){return result;}
        result.reserve(nPackets);
        for (auto &it = it0; it != it1; std::advance(it, 1))
        {
            result.push_back(*it);
        }
#ifndef NDEBUG
        assert(nPackets == static_cast<int> (result.size()));
#endif
        return result;
    }
    /// Resets the class
    void clear() noexcept
    {
        std::scoped_lock lock(mMutex);
        mCircularBuffer.clear();
        mName.clear();
        mNetwork.clear();
        mStation.clear();
        mChannel.clear();
        mLocationCode.clear();
        mMaxPackets = 0;
        mInitialized = false;
    }
    /// Return the capacity (max space) in the circular buffer
    int capacity() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return static_cast<int> (mCircularBuffer.capacity());
    }
    /// Return the size of the circular buffer
    int size() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return static_cast<int> (mCircularBuffer.size());
    }
    /// C'tor
    CircularBufferImpl() = default;
    /// Copy c'tor
    CircularBufferImpl(const CircularBufferImpl &cb)
    {
        std::scoped_lock lock(cb.mMutex);
        mCircularBuffer = cb.mCircularBuffer;
        mName = cb.mName;
        mNetwork = cb.mNetwork;
        mStation = cb.mStation;
        mChannel = cb.mChannel;
        mLocationCode = cb.mLocationCode;
        mMaxPackets = cb.mMaxPackets;
        mInitialized = cb.mInitialized; 
    }
    /// Move c'tor
    CircularBufferImpl(CircularBufferImpl &&cb) noexcept
    {
        std::scoped_lock lock(cb.mMutex);
        mCircularBuffer = std::move(cb.mCircularBuffer);
        mName = std::move(cb.mName);
        mNetwork = std::move(cb.mNetwork);
        mStation = std::move(cb.mStation);
        mChannel = std::move(cb.mChannel);
        mLocationCode = std::move(cb.mLocationCode);
        mMaxPackets = cb.mMaxPackets;
        mInitialized = cb.mInitialized;
    }
    /// Destructor
    ~CircularBufferImpl()
    {
        clear(); 
    }
///private:
    boost::circular_buffer<UDP::DataPacket> mCircularBuffer;
    std::string mName;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode; 
    mutable std::mutex mMutex;
    int mMaxPackets = 0;
    bool mInitialized = false;
};

/// C'tor
CircularBuffer::CircularBuffer() :
    pImpl(std::make_unique<CircularBufferImpl> ())
{
}

/// Copy c'tor
CircularBuffer::CircularBuffer(const CircularBuffer &cb)
{
    *this = cb;
}

/// Copy assignment
CircularBuffer& CircularBuffer::operator=(const CircularBuffer &cb)
{
    if (&cb == this){return *this;}
    pImpl = std::make_unique<CircularBufferImpl> (*cb.pImpl);
    return *this;
}

/// Move c'tor
CircularBuffer::CircularBuffer(CircularBuffer &&cb) noexcept
{
    *this = std::move(cb);
}

/// Move assignment
CircularBuffer& CircularBuffer::operator=(CircularBuffer &&cb) noexcept
{
    if (&cb == this){return *this;}
    pImpl = std::make_unique<CircularBufferImpl> (std::move(*cb.pImpl));
    return *this;
}

/// Initialize class
void CircularBuffer::initialize(
    const std::string &network, const std::string &station,
    const std::string &channel, const std::string &locationCode,
    const int maxPackets)
{
    clear();
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    if (::isEmpty(locationCode))
    {
        throw std::invalid_argument("Location code is empty");
    }
    if (maxPackets < 1)
    {
        throw std::invalid_argument("maxPackets " + std::to_string(maxPackets)
                                  + " must be positive");
    }
    pImpl->mCircularBuffer.rset_capacity(maxPackets);
    pImpl->mNetwork = network;
    pImpl->mStation = station;
    pImpl->mChannel = channel;
    pImpl->mLocationCode = locationCode;
    pImpl->mName = ::makeName(network, station, channel, locationCode);
    pImpl->mMaxPackets = maxPackets;
    pImpl->mInitialized = true;
}

std::string CircularBuffer::getNetwork() const
{
    if (!isInitialized()){throw std::invalid_argument("Class not initialized");}
    return pImpl->mNetwork;
}

std::string CircularBuffer::getStation() const
{
    if (!isInitialized()){throw std::invalid_argument("Class not initialized");}
    return pImpl->mStation;
}

std::string CircularBuffer::getChannel() const
{
    if (!isInitialized()){throw std::invalid_argument("Class not initialized");}
    return pImpl->mChannel;
}

std::string CircularBuffer::getLocationCode() const
{
    if (!isInitialized()){throw std::invalid_argument("Class not initialized");}
    return pImpl->mLocationCode;
}

/// Initialized?
bool CircularBuffer::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Circular buffer size 
int CircularBuffer::getNumberOfPackets() const noexcept
{
    return pImpl->size();
}

/// Circular buffer capacity
int CircularBuffer::getMaximumNumberOfPackets() const
{
    if (!isInitialized()){throw std::invalid_argument("Class not initialize");}
    return pImpl->capacity();
}

/// Reset class
void CircularBuffer::clear() noexcept
{
    pImpl->clear();
}

/// Add a packet
void CircularBuffer::addPacket(UDP::DataPacket &&packet)
{
    // Is this a valid packet?
    if (!::isValidPacket(packet))
    {   
        throw std::invalid_argument("Packet is invalid");
    }
    // Make the packet name
    auto packetName = ::makeName(packet);
    if (packetName != pImpl->mName)
    {   
        throw std::invalid_argument("Packet for " + packetName
                                  + " does not belong in buffer for "
                                  + pImpl->mName);
    }   
    pImpl->update(std::move(packet));
}

/// Add a packet
void CircularBuffer::addPacket(const UDP::DataPacket &packet)
{
    auto packetCopy = packet;
    addPacket(std::move(packetCopy));
}

/// Get earliest start time
std::chrono::microseconds CircularBuffer::getEarliestStartTime() const
{
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    auto startTime = pImpl->getEarliestStartTime();
    if (startTime == NAN_TIME)
    {
        throw std::runtime_error("No packets in buffer");
    }
    return startTime;
}

/// Destructor
CircularBuffer::~CircularBuffer() = default;

/// Get all the packets
std::vector<UDP::DataPacket> CircularBuffer::getPackets() const
{
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    auto packets = pImpl->getAllPackets();
#ifndef NDEBUG
    assert(std::is_sorted(packets.begin(), packets.end(),
           [](const UDP::DataPacket &lhs, const UDP::DataPacket &rhs)
           {
              return lhs.getStartTime() < rhs.getStartTime();
           }));
#endif
    return packets;
}

/// Get all packets from given time to now
std::vector<UDP::DataPacket> CircularBuffer::getPackets(
    const std::chrono::microseconds &t0) const
{
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    constexpr std::chrono::microseconds
        t1{std::numeric_limits<int64_t>::max()};
    auto packets = pImpl->getPackets(t0, t1);
#ifndef NDEBUG
    assert(std::is_sorted(packets.begin(), packets.end(),
           [](const UDP::DataPacket &lhs, const UDP::DataPacket &rhs)
           {
              return lhs.getStartTime() < rhs.getStartTime();
           }));
#endif
    return packets;
}

std::vector<UDP::DataPacket> CircularBuffer::getPackets(const double t0) const
{
    auto t0MicroSeconds = ::secondsToMicroSeconds(t0);
    return getPackets(t0MicroSeconds);
}


/// Get all packets from given time t0 to given time t1
std::vector<UDP::DataPacket> CircularBuffer::getPackets(
    const double t0, const double t1) const
{
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    if (t1 <= t0)
    {
        throw std::invalid_argument("t0 = " + std::to_string(t0)
                                  + " must be less than t1 = "
                                  + std::to_string(t1));
    }
    auto t0MicroSeconds = ::secondsToMicroSeconds(t0);
    auto t1MicroSeconds = ::secondsToMicroSeconds(t1);
    return getPackets(t0MicroSeconds, t1MicroSeconds);
}

std::vector<UDP::DataPacket> CircularBuffer::getPackets(
    const std::chrono::microseconds &t0,
    const std::chrono::microseconds &t1) const
{
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    if (t1 <= t0) 
    {
        throw std::invalid_argument("t0 = " + std::to_string(t0.count())
                                  + " must be less than t1 = "
                                  + std::to_string(t1.count()));
    }
    auto packets = pImpl->getPackets(t0, t1);
#ifndef NDEBUG
    assert(std::is_sorted(packets.begin(), packets.end(),
           [](const UDP::DataPacket &lhs, const UDP::DataPacket &rhs)
           {
              return lhs.getStartTime() < rhs.getStartTime();
           }));
#endif
    return packets;
}


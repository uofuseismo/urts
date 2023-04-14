#ifndef PRIVATE_SERVICES_SCALABLE_PACKET_CACHE_CIRCULAR_BUFFER_HPP
#define PRIVATE_SERVICES_SCALABLE_PACKET_CACHE_CIRCULAR_BUFFER_HPP
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

#define NAN_TIME std::chrono::microseconds{std::numeric_limits<int64_t>::lowest()}

template<class T>
class URTS::Services::Scalable::PacketCache::CircularBufferImpl
{
public:
    void update(T &&packet)
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
                                   [](const T &lhs, const T &rhs)
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
                                  [](const T &lhs, const T &rhs)
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
            //          [](const T &lhs, const T &rhs)
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
    [[nodiscard]] std::vector<T> getAllPackets() const
    {
        std::vector<T> result;
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
    [[nodiscard]] std::vector<T>
        getPackets(const std::chrono::microseconds t0MuS,
                   const std::chrono::microseconds t1MuS) const
    {
        std::vector<T> result;
        std::scoped_lock lock(mMutex);
        if (mCircularBuffer.empty()){return result;}
        auto it0 = std::upper_bound(mCircularBuffer.begin(),
                                    mCircularBuffer.end(), t0MuS,
                                    [](const std::chrono::microseconds t0MuS,
                                       const T &rhs)
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
                                      const T &rhs)
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
    mutable std::mutex mMutex;
    boost::circular_buffer<T> mCircularBuffer;
    std::string mName;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode; 
    int mMaxPackets{0};
    bool mInitialized{false};
};

#endif

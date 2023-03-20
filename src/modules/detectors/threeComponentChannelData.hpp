#ifndef PRIVATE_MODULES_DETECTORS_THREE_COMPONENT_CHANNEL_DATA_HPP
#define PRIVATE_MODULES_DETECTORS_THREE_COMPONENT_CHANNEL_DATA_HPP
#include <string>
#include <ostream>
#include <cmath>
#include "urts/database/aqms/channelData.hpp"

namespace
{

class ThreeComponentChannelData
{
public:
    /// @brief Constructor.
    ThreeComponentChannelData() = default;
    ThreeComponentChannelData(const ThreeComponentChannelData &) = default;
    ThreeComponentChannelData(ThreeComponentChannelData &&) noexcept = default;
    ThreeComponentChannelData& operator=(const ThreeComponentChannelData &) = default;
    ThreeComponentChannelData& operator=(ThreeComponentChannelData &&) noexcept = default;
    /// @brief Constructor
    ThreeComponentChannelData(const URTS::Database::AQMS::ChannelData &verticalChannel,
                         const URTS::Database::AQMS::ChannelData &northChannel,
                         const URTS::Database::AQMS::ChannelData &eastChannel)
    {
        setChannelData(verticalChannel, northChannel, eastChannel); 
    }    
    /// @brief Sets the channel data.
    void setChannelData(const URTS::Database::AQMS::ChannelData &verticalChannel,
                        const URTS::Database::AQMS::ChannelData &northChannel,
                        const URTS::Database::AQMS::ChannelData &eastChannel)
    {
        if (verticalChannel.getNetwork() != northChannel.getNetwork() ||
            verticalChannel.getNetwork() != eastChannel.getNetwork())
        {
            throw std::invalid_argument("Inconsistent networks");
        }
        if (verticalChannel.getStation() != northChannel.getStation() ||
            verticalChannel.getStation() != eastChannel.getStation())
        {
            throw std::invalid_argument("Inconsistent stations");
        }
        if (verticalChannel.getLocationCode() !=
            northChannel.getLocationCode() ||
            verticalChannel.getLocationCode() != eastChannel.getLocationCode())
        {
            throw std::invalid_argument("Inconsistent location codes");
        }
        if (std::abs(verticalChannel.getSamplingRate() 
                   - northChannel.getSamplingRate()) > 1.e-5 ||
            std::abs(verticalChannel.getSamplingRate()
                   - eastChannel.getSamplingRate()) > 1.e-5)
        {
            throw std::invalid_argument("Inconsistent sampling rates");
        }
        auto zChannel = verticalChannel.getChannel();
        auto nChannel = northChannel.getChannel();
        auto eChannel = eastChannel.getChannel();
        if (zChannel == nChannel || zChannel == eChannel)
        {
            throw std::invalid_argument("Duplicate channel");
        }
        zChannel.pop_back();
        nChannel.pop_back();
        eChannel.pop_back(); 
        if (zChannel != nChannel || zChannel != eChannel)
        {
            throw std::invalid_argument("Inconsistent channel codes");
        }
        mVerticalChannel = verticalChannel;
        mNorthChannel = northChannel;
        mEastChannel = eastChannel;
        mHaveChannelInformation = true;
        // Set the hash
        auto hashName = getNetwork() + "." 
                      + getStation() + "." 
                      + getVerticalChannel() + "." 
                      + getNorthChannel() + "." 
                      + getEastChannel() + "." 
                      + getLocationCode();
        mHash = std::hash<std::string>{} (hashName);
    }
    /// @result Vertical channel data.
    [[nodiscard]] const URTS::Database::AQMS::ChannelData
        &getVerticalChannelReference() const
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mVerticalChannel;
    }
    /// @result North channel data.
    [[nodiscard]] const URTS::Database::AQMS::ChannelData
        &getNorthChannelReference() const
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mNorthChannel;
    }
    /// @result East channel data.
    [[nodiscard]] const URTS::Database::AQMS::ChannelData
        &getEastChannelReference() const
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mEastChannel;
    }
    /// @result True indicates the channel information was set.
    [[nodiscard]] bool haveChannelInformation() const noexcept
    {
        return mHaveChannelInformation;
    }
    /// @result The network code.
    [[nodiscard]] std::string getNetwork() const 
    {
        if (!haveChannelInformation())
        {   
            throw std::runtime_error("Channel information not set");
        }
        return mVerticalChannel.getNetwork();
    }
    /// @result The station name.
    [[nodiscard]] std::string getStation() const 
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mVerticalChannel.getStation();
    }
    /// @result The vertical channel code.
    [[nodiscard]] std::string getVerticalChannel() const 
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mVerticalChannel.getChannel();
    }
    /// @result The north channel code.
    [[nodiscard]] std::string getNorthChannel() const
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mNorthChannel.getChannel();
    }
    /// @result The east channel code.
    [[nodiscard]] std::string getEastChannel() const
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mEastChannel.getChannel();
    }
    /// @result The location code.
    [[nodiscard]] std::string getLocationCode() const 
    {   
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mVerticalChannel.getLocationCode();
    }   
    /// @bresult The sampling rate.
    [[nodiscard]] double getNominalSamplingRate() const
    {
        if (!haveChannelInformation())
        {
            throw std::runtime_error("Channel information not set");
        }
        return mVerticalChannel.getSamplingRate();
    }
    /// @brief Resets the class and releases memory.
    void clear() noexcept
    {
        mVerticalChannel.clear();
        mNorthChannel.clear();
        mEastChannel.clear();
        mHash = std::hash<std::string>{} ("");
        mHaveChannelInformation = false;
    }
    /// @result The hash.
    [[nodiscard]] size_t getHash() const noexcept
    {
       return mHash;
    }
private:
    URTS::Database::AQMS::ChannelData mVerticalChannel;
    URTS::Database::AQMS::ChannelData mNorthChannel;
    URTS::Database::AQMS::ChannelData mEastChannel;
    size_t mHash{std::hash<std::string>{} ("")};
    bool mHaveChannelInformation{false};
};


[[nodiscard]] [[maybe_unused]]
bool operator<(const ThreeComponentChannelData &lhs,
               const ThreeComponentChannelData &rhs)
{
   return lhs.getHash() < rhs.getHash();
}

[[maybe_unused]]
std::ostream &operator<<(std::ostream &os,
                         const ThreeComponentChannelData &station)
{
    std::string name;
    if (station.haveChannelInformation())
    {
        name = station.getNetwork() + "."
             + station.getStation() + "."
             + station.getVerticalChannel() + "."
             + station.getNorthChannel() + "."
             + station.getEastChannel() + "."
             + station.getLocationCode();
    }
    return os << name;
}

}
#endif

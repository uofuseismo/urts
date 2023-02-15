#include <string>
#include "urts/database/aqms/channelData.hpp"
//#include "urts/database/aqms/stationData.hpp"
#include <time/utc.hpp>
#include <gtest/gtest.h>

namespace
{

using namespace URTS::Database::AQMS;

/*
TEST(DatabaseAQMS, StationData)
{
    const std::string network{"UU"};
    const std::string station{"IMU"};
    const std::string description{"Iron Mountain, UT, USA"};
    const double latitude = 38.63;
    const double longitude = -113.16;
    const double elevation = 1833;    
    Time::UTC tOn("2012-01-01T00:00:00");
    Time::UTC tOff("2021-01-01T00:00:00");
    Time::UTC loadDate("2012-01-01T00:00:01");
    StationData data;
    data.setNetwork(network);
    data.setStation(station);
    data.setLatitude(latitude);
    data.setLongitude(longitude);
    data.setElevation(elevation);
    data.setDescription(description);
    data.setOnOffDate(std::pair(tOn, tOff));
    data.setLoadDate(loadDate);

    StationData dataCopy(data);
    EXPECT_EQ(dataCopy.getNetwork(), network);
    EXPECT_EQ(dataCopy.getStation(), station);
    EXPECT_EQ(dataCopy.getDescription(), description);
    EXPECT_NEAR(dataCopy.getLatitude(),  latitude,  1.e-10);
    EXPECT_NEAR(dataCopy.getLongitude(), longitude, 1.e-10);
    EXPECT_NEAR(dataCopy.getElevation(), elevation, 1.e-10);
    EXPECT_EQ(dataCopy.getOnDate(),  tOn);
    EXPECT_EQ(dataCopy.getOffDate(), tOff);
    EXPECT_EQ(dataCopy.getLoadDate(), loadDate);
}
*/

TEST(DatabaseAQMS, ChannelData)
{
    const std::string network{"UU"};
    const std::string station{"IMU"};
    const std::string channel{"EHZ"};
    const std::string locationCode{"01"};
    const double samplingRate = 100;
    const double latitude = 38.63;
    const double longitude = -113.16;
    const double elevation = 1833;    
    const double dip =-90;
    const double azimuth = 45;  // Make up a bogus number for testing
    Time::UTC tOn("2012-01-01T00:00:00");
    Time::UTC tOff("2021-01-01T00:00:00");
    Time::UTC loadDate("2012-01-01T00:00:01");
    ChannelData data;
    data.setNetwork(network);
    data.setStation(station);
    data.setChannel(channel);
    data.setLocationCode(locationCode);
    data.setSamplingRate(samplingRate);
    data.setLatitude(latitude);
    data.setLongitude(longitude);
    data.setElevation(elevation);
    data.setDip(dip);
    data.setAzimuth(azimuth);
    data.setOnOffDate(std::pair(tOn, tOff));
    data.setLoadDate(loadDate);

    ChannelData dataCopy(data);
    EXPECT_EQ(dataCopy.getNetwork(), network);
    EXPECT_EQ(dataCopy.getStation(), station);
    EXPECT_EQ(dataCopy.getChannel(), channel);
    EXPECT_EQ(dataCopy.getLocationCode(), locationCode);
    EXPECT_NEAR(dataCopy.getLatitude(),     latitude,     1.e-10);
    EXPECT_NEAR(dataCopy.getLongitude(),    longitude,    1.e-10);
    EXPECT_NEAR(dataCopy.getElevation(),    elevation,    1.e-10);
    EXPECT_NEAR(dataCopy.getDip(),          dip,          1.e-10);
    EXPECT_NEAR(dataCopy.getAzimuth(),      azimuth,      1.e-10);
    EXPECT_NEAR(dataCopy.getSamplingRate(), samplingRate, 1.e-10);
    EXPECT_EQ(dataCopy.getOnDate(),  tOn);
    EXPECT_EQ(dataCopy.getOffDate(), tOff);
    EXPECT_EQ(dataCopy.getLoadDate(), loadDate);
}

}

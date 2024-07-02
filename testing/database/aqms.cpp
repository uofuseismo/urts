#include <string>
#include <cmath>
#include <chrono>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/aqms/stationData.hpp"
#include "urts/database/aqms/arrival.hpp"

using namespace URTS::Database::AQMS;

TEST_CASE("URTS::Database::AQMS", "[StationData]")
{
    const std::string network{"UU"};
    const std::string station{"IMU"};
    const std::string description{"Iron Mountain, UT, USA"};
    const double latitude = 38.63;
    const double longitude = -113.16;
    const double elevation = 1833;    
    std::chrono::microseconds tOn{1325376000000000}; // 2012-01-01T00:00:00
    std::chrono::microseconds tOff{1609459200000000}; // 2021-01-01T00:00:00
    std::chrono::microseconds loadDate{1325376001000000}; // 2012-01-01T00:00:01
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
    REQUIRE(dataCopy.getNetwork() == network);
    REQUIRE(dataCopy.getStation() == station);
    REQUIRE(dataCopy.getDescription() == description);
    REQUIRE(std::abs(dataCopy.getLatitude() - latitude) < 1.e-10);
    REQUIRE(std::abs(dataCopy.getLongitude() - longitude) < 1.e-10);
    REQUIRE(std::abs(dataCopy.getElevation() - elevation) < 1.e-10);
    REQUIRE(dataCopy.getOnDate() == tOn);
    REQUIRE(dataCopy.getOffDate() == tOff);
    REQUIRE(dataCopy.getLoadDate() == loadDate);
}

TEST_CASE("URTS::Database::AQMS", "[ChannelData]")
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
    std::chrono::microseconds tOn{1325376000000000}; // 2012-01-01T00:00:00
    std::chrono::microseconds tOff{1609459200000000}; // 2021-01-01T00:00:00
    std::chrono::microseconds loadDate{1325376001000000}; // 2012-01-01T00:00:01
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
    REQUIRE(dataCopy.getNetwork() == network);
    REQUIRE(dataCopy.getStation() == station);
    REQUIRE(dataCopy.getChannel() == channel);
    REQUIRE(dataCopy.getLocationCode() == locationCode);
    REQUIRE(std::abs(dataCopy.getLatitude() - latitude) < 1.e-10);
    REQUIRE(std::abs(dataCopy.getLongitude() - longitude) < 1.e-10);
    REQUIRE(std::abs(dataCopy.getElevation() - elevation) < 1.e-10);
    REQUIRE(std::abs(dataCopy.getDip() - dip) < 1.e-10);
    REQUIRE(std::abs(dataCopy.getAzimuth() - azimuth) < 1.e-10);
    REQUIRE(std::abs(dataCopy.getSamplingRate() - samplingRate) < 1.e-10);
    REQUIRE(dataCopy.getOnDate() == tOn);
    REQUIRE(dataCopy.getOffDate() == tOff);
    REQUIRE(dataCopy.getLoadDate() == loadDate);
}

TEST_CASE("URTS::Database::AQMS", "Arrival")
{
    const std::string authority{"UU"};
    const std::string network{"UU"};
    const std::string station{"BHUT"};
    const std::string channel{"HHZ"};
    const std::string locationCode{"01"};
    const std::string phase{"P"};
    const std::string subSource{"ML1"};
    const std::chrono::microseconds time{123456789};
    const Arrival::FirstMotion firstMotion{Arrival::FirstMotion::Up};
    const Arrival::ReviewFlag reviewFlag{Arrival::ReviewFlag::Human};
    const int64_t identifier{9423};
    const double quality{0.5};
    Arrival arrival;
    arrival.setAuthority(authority);
    arrival.setSubSource(subSource);
    arrival.setNetwork(network);
    arrival.setStation(station);
    arrival.setSEEDChannel(channel);
    arrival.setLocationCode(locationCode);
    arrival.setPhase(phase);
    arrival.setTime(time);
    arrival.setFirstMotion(firstMotion);
    arrival.setReviewFlag(reviewFlag);
    arrival.setQuality(quality);
    arrival.setIdentifier(identifier);
    SECTION("Copy")
    {
        Arrival copy{arrival}; 
        REQUIRE(copy.getAuthority() == authority);
        REQUIRE(*copy.getNetwork() == network);
        REQUIRE(copy.getStation() == station);
        REQUIRE(*copy.getSEEDChannel() == channel);
        REQUIRE(*copy.getLocationCode() == locationCode);
        REQUIRE(*copy.getPhase() == phase);
        REQUIRE(std::abs(copy.getTime() - time.count()*1.e-6) < 1.e-14);
        REQUIRE(copy.getFirstMotion() == firstMotion);
        REQUIRE(*copy.getReviewFlag() == reviewFlag);
        REQUIRE(std::abs(*copy.getQuality() - quality) < 1.e-14);
        REQUIRE(*copy.getSubSource() == subSource);
        REQUIRE(*copy.getIdentifier() == identifier);
    }
    
}

#include <iostream>
#include <string>
#include <cmath>
#include <chrono>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/aqms/stationData.hpp"
#include "urts/database/aqms/arrival.hpp"
#include "urts/database/aqms/origin.hpp"
#include "urts/database/aqms/assocaro.hpp"
#include "urts/database/aqms/event.hpp"

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
std::cout << toInsertString(arrival) << std::endl; 
}

TEST_CASE("URTS::Database::AQMS", "[Origin]")
{
    const std::chrono::microseconds time{1720031875794000};
    const double latitude{41.};
    const double longitude{-112.};
    const double depth{8};
    const double distance{11.2};
    const double gap{123.};
    const double wrmse{0.13};
    const std::string algorithm{"MAssociator"};
    const std::string subSource{"ML1"};
    const std::string authority{"UU"};
    const int64_t identifier{83832};
    const int64_t eventIdentifier{9293023};
    const int64_t magIdentifier{323};
    const int64_t mecIdentifier{48};
    const auto geographicType = Origin::GeographicType::Local;
    const auto reviewFlag = Origin::ReviewFlag::Human;
    Origin origin;
    origin.setAuthority(authority);
    origin.setIdentifier(identifier);
    origin.setEventIdentifier(eventIdentifier);
    origin.setLatitude(latitude);
    origin.setLongitude(longitude);
    origin.setTime(time);
    origin.setDepth(depth);
    origin.setSubSource(subSource);
    origin.setAlgorithm(algorithm);
    origin.setGap(gap);
    origin.setDistanceToNearestStation(distance);
    origin.setGeographicType(geographicType);
    origin.setPreferredMagnitudeIdentifier(magIdentifier);
    origin.setPreferredMechanismIdentifier(mecIdentifier);
    origin.setReviewFlag(reviewFlag);
    origin.setWeightedRootMeanSquaredError(wrmse);
    SECTION("Copy")
    {
        Origin copy{origin};
        REQUIRE(copy.getAuthority() == authority);
        REQUIRE(copy.getIdentifier() == identifier);
        REQUIRE(copy.getEventIdentifier() == eventIdentifier);
        REQUIRE(std::abs(copy.getTime() - time.count()*1.e-6) < 1.e-8);
        REQUIRE(std::abs(copy.getLatitude() - latitude) < 1.e-13);
        REQUIRE(std::abs(copy.getLongitude() - longitude) < 1.e-13);
        REQUIRE(std::abs(*copy.getDepth() - depth) < 1.e-14);
        REQUIRE(!copy.isBogus()); 
        REQUIRE(*copy.getGeographicType() == geographicType);
        REQUIRE(std::abs(*copy.getGap() - gap) < 1.e-13);
        REQUIRE(std::abs(*copy.getDistanceToNearestStation() - distance) < 1.e-13);
        REQUIRE(*copy.getPreferredMagnitudeIdentifier() == magIdentifier);
        REQUIRE(*copy.getPreferredMechanismIdentifier() == mecIdentifier);
        REQUIRE(*copy.getReviewFlag() == reviewFlag);
        REQUIRE(std::abs(*copy.getWeightedRootMeanSquaredError() - wrmse) < 1.e-13);
    }
}

TEST_CASE("URTS::Database::AQMS", "[Event]")
{
    const std::string authority{"UU"};
    const std::string subSource{"ML1"};
    const int64_t identifier{83232};
    const int64_t originIdentifier{992323};
    const int64_t magnitudeIdentifier{29932};
    const int64_t mechanismIdentifier{2323};
    const int64_t commentIdentifier{8};
    const Event::Type type{Event::Type::Earthquake};
    const int version{32};
    Event event;
 
    event.setIdentifier(identifier);
    event.setAuthority(authority);
    event.setPreferredOriginIdentifier(originIdentifier);
    event.setPreferredMagnitudeIdentifier(magnitudeIdentifier);
    event.setPreferredMechanismIdentifier(mechanismIdentifier);
    event.setCommentIdentifier(commentIdentifier);
    event.setType(type);
    event.setVersion(version);
    event.setSubSource(subSource);
    event.setSelectFlag();

    SECTION("Copy")
    {
        Event copy{event};
        REQUIRE(copy.getIdentifier() == identifier);
        REQUIRE(copy.getAuthority() == authority);
        REQUIRE(*copy.getPreferredOriginIdentifier() == originIdentifier);
        REQUIRE(*copy.getPreferredMagnitudeIdentifier() == magnitudeIdentifier);
        REQUIRE(*copy.getPreferredMechanismIdentifier() == mechanismIdentifier);
        REQUIRE(*copy.getCommentIdentifier() == commentIdentifier);
        REQUIRE(copy.getType() == type);
        REQUIRE(copy.getVersion() == version);
        REQUIRE(*copy.getSubSource() == subSource);
        REQUIRE(copy.getSelectFlag());
    }

std::cout << toInsertString(event) << std::endl;

}

TEST_CASE("URTS::Database::AQMS", "[AssocArO]")
{
    const int64_t originIdentifier{83232};
    const int64_t arrivalIdentifier{292233};
    const std::string authority{"UU"};
    const std::string subSource{"ML1"};
    const std::string phase{"P"};
    const double sourceReceiverDistance{10};
    const double azimuth{37};
    const double inputWeight{0.75};
    const double residual{-0.02};
    const AssocArO::ReviewFlag reviewFlag{AssocArO::ReviewFlag::Automatic};

    AssocArO assocaro;
    assocaro.setOriginIdentifier(originIdentifier);
    assocaro.setArrivalIdentifier(arrivalIdentifier);
    assocaro.setAuthority(authority); 
    assocaro.setSubSource(subSource);
    assocaro.setPhase(phase);
    assocaro.setSourceReceiverDistance(sourceReceiverDistance);
    assocaro.setSourceToReceiverAzimuth(azimuth);
    assocaro.setInputWeight(inputWeight);
    assocaro.setTravelTimeResidual(residual);
    assocaro.setReviewFlag(reviewFlag);

    SECTION("Copy")
    {
        AssocArO copy{assocaro};
        REQUIRE(copy.getOriginIdentifier() == originIdentifier);
        REQUIRE(copy.getArrivalIdentifier() == arrivalIdentifier);
        REQUIRE(copy.getAuthority() == authority);
        REQUIRE(*copy.getSubSource() == subSource);
        REQUIRE(*copy.getPhase() == phase);
        REQUIRE(std::abs(*copy.getSourceReceiverDistance() - sourceReceiverDistance) < 1.e-10);
        REQUIRE(std::abs(*copy.getSourceToReceiverAzimuth() - azimuth) < 1.e-10);
        REQUIRE(std::abs(*copy.getInputWeight() - inputWeight) < 1.e-10);
        REQUIRE(std::abs(*copy.getTravelTimeResidual() - residual) < 1.e-10);
        REQUIRE(*copy.getReviewFlag() == reviewFlag);
    }
std::cout << toInsertString(assocaro) << std::endl;
}


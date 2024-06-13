#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "urts/broadcasts/internal/origin/origin.hpp"
#include "urts/broadcasts/internal/origin/arrival.hpp"
#include "urts/broadcasts/internal/origin/subscriberOptions.hpp"
#include "urts/broadcasts/internal/origin/publisherOptions.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::Origin"

using namespace URTS::Broadcasts::Internal::Origin;

namespace
{

}

TEST_CASE("URTS::Broadcasts::Internal::Origin", "[arrival]")
{
/*
    const double percentile{95};
    const std::chrono::microseconds perturbation{1500};

    UncertaintyBound bound;
    REQUIRE_NOTHROW(bound.setPercentile(percentile));
    bound.setPerturbation(perturbation);

    UncertaintyBound copy(bound);
    REQUIRE(std::abs(bound.getPercentile() - percentile) < 1.e-14);
    REQUIRE(bound.getPerturbation() == perturbation);
 
    SECTION("clear and check defaults")
    {   
        bound.clear();
        REQUIRE(std::abs(bound.getPercentile() - 50) < 1.e-14);
        REQUIRE(bound.getPerturbation() == std::chrono::microseconds {0});
    }   
*/
}

TEST_CASE("URTS::Broadcasts::Internal::Arrival", "[arrival]")
{
    URTS::Broadcasts::Internal::Pick::UncertaintyBound lowerBound, upperBound;
    const std::chrono::microseconds perturbationLow{-1500};
    const std::chrono::microseconds perturbationHigh{1500};
    REQUIRE_NOTHROW(lowerBound.setPercentile(5));
    lowerBound.setPerturbation(perturbationLow);
    REQUIRE_NOTHROW(upperBound.setPercentile(95));
    upperBound.setPerturbation(perturbationHigh);

    Arrival arrival;
    arrival.setIdentifier(2312);
    arrival.setTime(std::chrono::microseconds {500}); 
    arrival.setNetwork("UU");
    arrival.setStation("BGU");
    arrival.setChannel("HHZ");
    arrival.setLocationCode("01");
    arrival.setPhase(Arrival::Phase::S);
    arrival.setOriginalChannels(std::vector<std::string> {"HHZ", "HNP"});
    arrival.setProcessingAlgorithms(std::vector<std::string> {"A", "B"});
    arrival.setFirstMotion(Arrival::FirstMotion::Up);
    arrival.setReviewStatus(Arrival::ReviewStatus::Manual);
    arrival.setLowerAndUpperUncertaintyBound(std::pair {lowerBound, upperBound});
 
    REQUIRE(arrival.getIdentifier() == 2312);
    REQUIRE(arrival.getTime() == std::chrono::microseconds {500});
    REQUIRE(arrival.getNetwork() == "UU");
    REQUIRE(arrival.getStation() == "BGU");
    REQUIRE(arrival.getChannel() == "HHZ");
    REQUIRE(arrival.getLocationCode() == "01");
    REQUIRE(arrival.getPhase() == Arrival::Phase::S);
    auto algorithmsBack = arrival.getProcessingAlgorithms();
    REQUIRE(algorithmsBack.at(0) == "A");
    REQUIRE(algorithmsBack.at(1) == "B");
    auto channelsBack = arrival.getOriginalChannels();
    REQUIRE(channelsBack.at(0) == "HHZ");
    REQUIRE(channelsBack.at(1) == "HNP");
    REQUIRE(arrival.getFirstMotion() == Arrival::FirstMotion::Up);
    REQUIRE(arrival.getReviewStatus() == Arrival::ReviewStatus::Manual);
    auto bounds = arrival.getLowerAndUpperUncertaintyBound();
    REQUIRE(std::abs(bounds->first.getPercentile() - 5) < 1.e-10);
    REQUIRE(std::abs(bounds->second.getPercentile() - 95) < 1.e-10);
    REQUIRE(bounds->first.getPerturbation() == perturbationLow);
    REQUIRE(bounds->second.getPerturbation() == perturbationHigh);

    SECTION("clear")
    {
        arrival.clear();
        REQUIRE(arrival.getFirstMotion() == Arrival::FirstMotion::Unknown);
        REQUIRE(arrival.getReviewStatus() == Arrival::ReviewStatus::Automatic);
        REQUIRE(arrival.getLowerAndUpperUncertaintyBound() == std::nullopt);
    } 
}

TEST_CASE("URTS::Broadcasts::Internal::Origin", "[origin]")
{
    int64_t identifier{3923};
    double latitude{42.2};
    double longitude{-111.9};
    double depth{6400};
    std::chrono::microseconds time{1717168659000000};
    std::vector<std::string> algorithms{"mAssociate"};
    Origin::ReviewStatus reviewStatus{Origin::ReviewStatus::Manual};

    Arrival arrival1;
    arrival1.setNetwork("UU");
    arrival1.setStation("HVU");
    arrival1.setChannel("HHZ");
    arrival1.setLocationCode("01");
    arrival1.setPhase(Arrival::Phase::P);
    arrival1.setTime(time + std::chrono::microseconds {1000000});
    arrival1.setIdentifier(1);
    arrival1.setFirstMotion(Arrival::FirstMotion::Up);
    arrival1.setReviewStatus(Arrival::ReviewStatus::Automatic);
    arrival1.setOriginIdentifier(identifier);
    arrival1.setResidual(-0.01);
 
    Arrival arrival2;
    arrival2.setNetwork("UU");
    arrival2.setStation("HVU");
    arrival2.setChannel("HHT");
    arrival2.setLocationCode("01");
    arrival2.setPhase(Arrival::Phase::S);
    arrival2.setTime(time + std::chrono::microseconds {2000000});
    arrival2.setIdentifier(2);
    arrival2.setFirstMotion(Arrival::FirstMotion::Unknown);
    arrival2.setReviewStatus(Arrival::ReviewStatus::Manual); 
    arrival2.setOriginalChannels(
        std::vector<std::string> {"HHZ", "HHN", "HHE"});
    arrival2.setOriginIdentifier(identifier);
    arrival2.setResidual(0.01);
    std::vector<Arrival> arrivals{arrival1, arrival2};
 
    Origin origin;
    REQUIRE_NOTHROW(origin.setLatitude(latitude));
    origin.setLongitude(longitude);
    REQUIRE_NOTHROW(origin.setDepth(depth));
    origin.setTime(time);
    origin.setIdentifier(identifier);
    origin.setAlgorithms(algorithms);
    origin.setReviewStatus(reviewStatus);
    REQUIRE_NOTHROW(origin.setArrivals(arrivals));

    REQUIRE(origin.getMessageType() == MESSAGE_TYPE);
    REQUIRE(std::abs(origin.getLatitude() - latitude) < 1.e-8);
    REQUIRE(std::abs(origin.getLongitude() - longitude) < 1.e-8);
    REQUIRE(std::abs(origin.getDepth() - depth) < 1.e-8);
    REQUIRE(origin.getTime() == time);
    REQUIRE(origin.getIdentifier() == identifier);
    REQUIRE(origin.getAlgorithms() == algorithms);
    REQUIRE(origin.getReviewStatus() == reviewStatus);

    SECTION("from message")
    {
        Origin copy;
        copy.fromMessage(origin.toMessage());

        REQUIRE(copy.getMessageType() == MESSAGE_TYPE);
        REQUIRE(std::abs(copy.getLatitude() - latitude) < 1.e-8);
        REQUIRE(std::abs(copy.getLongitude() - longitude) < 1.e-8);
        REQUIRE(std::abs(copy.getDepth() - depth) < 1.e-8);
        REQUIRE(copy.getTime() == time);
        REQUIRE(copy.getIdentifier() == identifier);
        REQUIRE(copy.getAlgorithms() == algorithms);
        REQUIRE(copy.getReviewStatus() == reviewStatus);

        auto arrivalsBack = copy.getArrivals();
        REQUIRE(arrivalsBack.size() == arrivals.size());
        for (int i = 0; i < static_cast<int> (arrivals.size()); ++i)
        {
            REQUIRE(arrivalsBack[i].getIdentifier() ==
                    arrivals[i].getIdentifier());
            REQUIRE(arrivalsBack[i].getNetwork() == arrivals[i].getNetwork());
            REQUIRE(arrivalsBack[i].getStation() == arrivals[i].getStation());
            REQUIRE(arrivalsBack[i].getChannel() == arrivals[i].getChannel());
            REQUIRE(arrivalsBack[i].getLocationCode() ==
                    arrivals[i].getLocationCode());
            REQUIRE(arrivalsBack[i].getPhase() == arrivals[i].getPhase());
            REQUIRE(arrivalsBack[i].getFirstMotion() ==
                    arrivals[i].getFirstMotion());
            REQUIRE(arrivalsBack[i].getTime() == arrivals[i].getTime());
            REQUIRE(arrivalsBack[i].getIdentifier() ==
                    arrivals[i].getIdentifier());
            REQUIRE(arrivalsBack[i].getOriginIdentifier() ==
                    arrivals[i].getOriginIdentifier());
            REQUIRE(arrivalsBack[i].getReviewStatus() ==
                    arrivals[i].getReviewStatus());
            REQUIRE(arrivalsBack[i].getOriginalChannels() ==
                    arrivals[i].getOriginalChannels());
            REQUIRE(std::abs(*arrivalsBack[i].getResidual()
                           - *arrivals[i].getResidual()) < 1.e-10);
        }
    }
}

TEST_CASE("URTS::Broadcasts::Internal::Origin", "[subscriber_options]")
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int recvHWM{106};
    const std::chrono::milliseconds recvTimeOut{145};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    SubscriberOptions options;
    REQUIRE_NOTHROW(options.setAddress(address));
    REQUIRE_NOTHROW(options.setHighWaterMark(recvHWM));
    REQUIRE_NOTHROW(options.setTimeOut(recvTimeOut));
    REQUIRE_NOTHROW(options.setZAPOptions(zapOptions));

    SubscriberOptions copy(options);
    REQUIRE(options.getAddress() == address);
    REQUIRE(options.getHighWaterMark() == recvHWM);
    REQUIRE(options.getTimeOut() == recvTimeOut);
    REQUIRE(options.getZAPOptions().getSecurityLevel() ==
              zapOptions.getSecurityLevel());    

    SECTION("clear and check defaults")
    {   
        options.clear();
        REQUIRE(!options.haveAddress());
        REQUIRE(options.getHighWaterMark() == 8192); 
        REQUIRE(options.getTimeOut() == std::chrono::milliseconds {10});
        REQUIRE(options.getZAPOptions().getSecurityLevel() ==
                UMPS::Authentication::SecurityLevel::Grasslands);
    }   
}

TEST_CASE("URTS::Broadcasts::Internal::Origin", "[publisher_options]")
{
    PublisherOptions options;
    const std::string address{"tcp://127.0.0.1:8080"};
    const int sendHWM{113};
    const std::chrono::milliseconds sendTimeOut{155};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    REQUIRE_NOTHROW(options.setAddress(address));
    REQUIRE_NOTHROW(options.setHighWaterMark(sendHWM));
    REQUIRE_NOTHROW(options.setTimeOut(sendTimeOut));
    REQUIRE_NOTHROW(options.setZAPOptions(zapOptions));

    PublisherOptions copy(options); 
    REQUIRE(options.getAddress() == address);
    REQUIRE(options.getHighWaterMark() == sendHWM);
    REQUIRE(options.getTimeOut() == sendTimeOut);
    REQUIRE(options.getZAPOptions().getSecurityLevel() ==
              zapOptions.getSecurityLevel());

    SECTION("clear and check defaults")
    {
        options.clear();
        REQUIRE(!options.haveAddress());
        REQUIRE(options.getHighWaterMark() == 8192);
        REQUIRE(options.getTimeOut() == std::chrono::milliseconds {1000});
        REQUIRE(options.getZAPOptions().getSecurityLevel() ==
                UMPS::Authentication::SecurityLevel::Grasslands);
    }
}

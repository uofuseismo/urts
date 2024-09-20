#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/services/scalable/locators/uLocator/arrival.hpp"
#include "urts/services/scalable/locators/uLocator/origin.hpp"
#include "urts/services/scalable/locators/uLocator/locationRequest.hpp"
#include "urts/services/scalable/locators/uLocator/locationResponse.hpp"
#include "urts/services/scalable/locators/uLocator/serviceOptions.hpp"
/*
#include "urts/services/scalable/associators/massociate/service.hpp"
*/
#include "urts/services/scalable/locators/uLocator/requestorOptions.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#define ORIGIN_IDENTIFIER 7941
#define ORIGIN_TIME_SECONDS 1725511003.787405
#define ORIGIN_LATITUDE 44.741678
#define ORIGIN_LONGITUDE -111.114317
#define ORIGIN_DEPTH 12176.528746

namespace URTSLoc = URTS::Services::Scalable::Locators::ULocator;

namespace
{
URTSLoc::Arrival toArrival(const std::string &network,
                           const std::string &station,
                           const std::string &phase,
                           const double time,
                           const double standardError,
                           const int64_t identifier,
                           const double travelTime)
{
    URTSLoc::Arrival result;
    result.setNetwork(network);
    result.setStation(station);
    if (phase == "P")
    {
        result.setPhase(URTSLoc::Arrival::Phase::P);
    }
    else
    {
        result.setPhase(URTSLoc::Arrival::Phase::S);
    }
    result.setTime(time);
    result.setStandardError(standardError);
    result.setIdentifier(identifier);
    return result;
}

std::vector<URTSLoc::Arrival> createArrivals()
{
    std::vector<URTSLoc::Arrival> arrivals;
    arrivals.push_back(::toArrival("WY", "YGC", "P", 1725511007.527704, 0.11, 1, 2.0));
    arrivals.push_back(::toArrival("WY", "YMC", "P", 1725511007.606693, 0.12, 2, 2.1));
    arrivals.push_back(::toArrival("WY", "YDC", "P", 1725511007.895112, 0.14, 4, 2.2));
    arrivals.push_back(::toArrival("WY", "YMR", "P", 1725511008.179964, 0.15, 5, 2.4));
    arrivals.push_back(::toArrival("WY", "YWB", "P", 1725511008.636633, 0.16, 6, 2.6));
    arrivals.push_back(::toArrival("WY", "YMC", "S", 1725511009.404663, 0.21, 7, 3.1));
    arrivals.push_back(::toArrival("WY", "YDC", "S", 1725511010.032740, 0.22, 8, 3.2));
    return arrivals;
}

URTSLoc::Origin createOrigin()
{
    URTSLoc::Origin origin;
    origin.setIdentifier(7941);
    origin.setTime(1725511003.787405);
    origin.setLatitude(44.741678);
    origin.setLongitude(-111.114317);
    origin.setDepth(12176.528746);
    origin.setArrivals(::createArrivals());
    return origin;
}

bool equals(const URTSLoc::Arrival &lhs, const URTSLoc::Arrival &rhs)
{
    if (lhs.getNetwork() != rhs.getNetwork()){return false;}
    if (lhs.getStation() != rhs.getStation()){return false;}
    if (lhs.getPhase() != rhs.getPhase()){return false;}
    if (lhs.getTime() != rhs.getTime()){return false;}
    if (lhs.getIdentifier() && rhs.getIdentifier())
    {
        if (*lhs.getIdentifier() != *rhs.getIdentifier()){return false;}
    }
    if (lhs.getTravelTime() && rhs.getTravelTime())
    {
        if (std::abs(*lhs.getTravelTime() - *rhs.getTravelTime()) > 1.e-6){return false;}
    }
    if (lhs.getStandardError() && rhs.getStandardError())
    {
        if (std::abs(*lhs.getStandardError() - *rhs.getStandardError()) > 1.e-6){return false;}
    }
    return true;
}

}

TEST_CASE("URTS::Services::Scalable::Locators::uLocator", "[models]")
{
    SECTION("Arrival")
    {
        const std::string network{"WY"};
        const std::string station{"YDC"};
        const URTSLoc::Arrival::Phase phase{URTSLoc::Arrival::Phase::P};
        const double time{1725562581.507};
        const std::chrono::microseconds timeMuS{static_cast<int64_t> (std::round(time*1.e6))};
        const double travelTime{4};
        const double standardError{0.12};
        const int64_t identifier{498423};
        URTSLoc::Arrival arrival;

        arrival.setNetwork(network);
        arrival.setStation(station);
        arrival.setTime(time);
        arrival.setPhase(phase);
        arrival.setIdentifier(identifier);
        arrival.setTravelTime(travelTime);
        arrival.setStandardError(standardError);

        CHECK(arrival.getNetwork() == network);
        CHECK(arrival.getStation() == station);
        CHECK(arrival.getTime() == timeMuS);
        CHECK(*arrival.getIdentifier() == identifier);
        CHECK_THAT(*arrival.getTravelTime(),
                   Catch::Matchers::WithinRel(travelTime, 1.e-12));
        CHECK_THAT(*arrival.getStandardError(),
                   Catch::Matchers::WithinRel(standardError, 1.e-12));
        REQUIRE(::equals(arrival, arrival)); 
    }

    SECTION("Origin")
    {
        URTSLoc::Origin origin;
        origin.setIdentifier(ORIGIN_IDENTIFIER);
        origin.setTime(ORIGIN_TIME_SECONDS);
        origin.setLatitude(ORIGIN_LATITUDE);
        origin.setLongitude(ORIGIN_LONGITUDE);
        origin.setDepth(ORIGIN_DEPTH);
        origin.setArrivals(::createArrivals());

        CHECK(origin.getIdentifier() == ORIGIN_IDENTIFIER);
        CHECK_THAT(origin.getTime().count()*1.e-6, Catch::Matchers::WithinRel(ORIGIN_TIME_SECONDS, 1.e-6));
        CHECK_THAT(origin.getLatitude(), Catch::Matchers::WithinRel(ORIGIN_LATITUDE, 1.e-6));
        CHECK_THAT(origin.getLongitude(), Catch::Matchers::WithinRel(ORIGIN_LONGITUDE, 1.e-6));
        CHECK_THAT(origin.getDepth(), Catch::Matchers::WithinRel(ORIGIN_DEPTH, 1.e-6));
        auto arrivals = ::createArrivals();
        auto arrivalsBack = origin.getArrivals();
        REQUIRE(arrivals.size() == arrivalsBack.size());
        for (const auto &arrival : arrivalsBack)
        {
            bool matched{false};
            for (const auto &refArrival : arrivals)
            {
                if (::equals(arrival, refArrival))
                {
                    matched = true;
                    break;
                }
                /*
                if (arrival.getNetwork() == refArrival.getNetwork() &&
                    arrival.getStation() == refArrival.getStation() &&
                    arrival.getPhase() == refArrival.getPhase() &&
                    arrival.getIdentifier() == refArrival.getIdentifier() &&
                    std::abs(*arrival.getStandardError() - *refArrival.getStandardError()) < 1.e-10 &&
                    arrival.getTime() == refArrival.getTime())
                {
                    matched = true;
                    break;
                }
                */
            }
            CHECK(matched);
        }
    }

    SECTION("LocationRequest")
    {
        const int64_t identifier{48232};
        const auto locationStrategy{URTSLoc::LocationRequest::LocationStrategy::FreeSurface};

        URTSLoc::LocationRequest request; 
        request.setArrivals(::createArrivals());
        request.setIdentifier(identifier);
        request.setLocationStrategy(locationStrategy);
        auto message = request.toMessage();

        
        URTSLoc::LocationRequest copy;
        copy.fromMessage(message);
        CHECK(copy.getMessageType() == "URTS::Services::Scalable::Locators::ULocator::LocationRequest");
        CHECK(copy.getIdentifier() == identifier);       
        CHECK(copy.getLocationStrategy() == locationStrategy);

        auto arrivals = ::createArrivals();
        auto arrivalsBack = copy.getArrivals();
        REQUIRE(arrivals.size() == arrivalsBack.size());
        for (const auto &arrival : arrivalsBack)
        {
            bool matched{false};
            for (const auto &refArrival : arrivals)
            {
                if (::equals(arrival, refArrival))
                {
                    matched = true;
                    break;
                }
            }
            CHECK(matched);
        }
    }

    SECTION("LocationResponse")
    {
        auto origin = createOrigin();
        auto arrivals = origin.getArrivals();
        for (auto &arrival : arrivals)
        {
            auto observedTravelTime = arrival.getTime() - origin.getTime();
            arrival.setTravelTime(std::round(observedTravelTime.count()*1.e-6));
        }
        origin.setArrivals(arrivals);
 
        const int64_t identifier{48232};
        const auto returnCode{URTSLoc::LocationResponse::ReturnCode::Success};
        URTSLoc::LocationResponse response;
        response.setIdentifier(identifier);
        response.setOrigin(origin);
        response.setReturnCode(returnCode);
        auto message = response.toMessage();

        URTSLoc::LocationResponse copy;
        copy.fromMessage(message);
        REQUIRE(copy.getOrigin());
        auto originBack = *copy.getOrigin();
        CHECK(originBack.getIdentifier() == ORIGIN_IDENTIFIER);
        CHECK_THAT(originBack.getTime().count()*1.e-6, Catch::Matchers::WithinRel(ORIGIN_TIME_SECONDS, 1.e-6));
        CHECK_THAT(originBack.getLatitude(), Catch::Matchers::WithinRel(ORIGIN_LATITUDE, 1.e-6));
        CHECK_THAT(originBack.getLongitude(), Catch::Matchers::WithinRel(ORIGIN_LONGITUDE, 1.e-6));
        CHECK_THAT(originBack.getDepth(), Catch::Matchers::WithinRel(ORIGIN_DEPTH, 1.e-6));
        auto arrivalsBack = originBack.getArrivals();
        
        REQUIRE(arrivals.size() == arrivalsBack.size());
        for (const auto &arrival : arrivalsBack)
        {
            bool matched{false};
            for (const auto &refArrival : arrivals)
            {
                if (::equals(arrival, refArrival))
                {
                    matched = true;
                    break;
                }
            }
            CHECK(matched);
        }
    }

    SECTION("RequestorOptions")
    {
        const std::string address{"tcp://127.0.0.1:5550"};
        const int sendHWM{105};
        const int recvHWM{106};
        const std::chrono::milliseconds sendTimeOut{120};
        const std::chrono::milliseconds recvTimeOut{145};
        UMPS::Authentication::ZAPOptions zapOptions;
        zapOptions.setStrawhouseClient();

        URTSLoc::RequestorOptions options;
        CHECK_NOTHROW(options.setAddress(address));
        CHECK_NOTHROW(options.setSendHighWaterMark(sendHWM));
        CHECK_NOTHROW(options.setReceiveHighWaterMark(recvHWM));
        options.setSendTimeOut(sendTimeOut);
        options.setReceiveTimeOut(recvTimeOut);
        options.setZAPOptions(zapOptions);
    
        SECTION("from copy constructor")
        {   
            URTSLoc::RequestorOptions copy(options);
            CHECK(copy.getAddress() == address);
            CHECK(copy.getSendHighWaterMark() == sendHWM);
            CHECK(copy.getReceiveHighWaterMark() == recvHWM);
            CHECK(copy.getSendTimeOut() == sendTimeOut); 
            CHECK(copy.getReceiveTimeOut() == recvTimeOut);
            CHECK(copy.getZAPOptions().getSecurityLevel() ==
                  zapOptions.getSecurityLevel());
        }   

        SECTION("clear and check defaults")
        {   
            options.clear();
            CHECK(options.getSendHighWaterMark() == 2048);
            CHECK(options.getReceiveHighWaterMark() == 0); 
            CHECK(options.getSendTimeOut() ==
                  std::chrono::milliseconds {0});
            REQUIRE(options.getReceiveTimeOut() ==
                    std::chrono::milliseconds {60000});
            zapOptions.setGrasslandsClient();
            CHECK(options.getZAPOptions().getSecurityLevel() ==
                  zapOptions.getSecurityLevel());
        }
    }

    SECTION("ServiceOptions")
    {
        URTSLoc::ServiceOptions serviceOptions;  
        serviceOptions.setRegion(URTSLoc::ServiceOptions::Region::Utah);
        serviceOptions.setNorm(URTSLoc::ServiceOptions::Norm::Lp, 1.1);
        serviceOptions.setOriginTimeSearchWindow(53);
        serviceOptions.setNumberOfParticles(83);
        serviceOptions.setNumberOfGenerations(12);
        serviceOptions.setMaximumSearchDepth(50000);
        serviceOptions.setHorizontalRefinement(62000);
        serviceOptions.setNumberOfFunctionEvaluations(932);
        serviceOptions.setReceiveHighWaterMark(500);
        serviceOptions.setSendHighWaterMark(1000);

        SECTION("service options copy")
        {   
            URTSLoc::ServiceOptions optionsCopy{serviceOptions}; 
            auto [norm, pNorm] = optionsCopy.getNorm();
            CHECK(norm == URTSLoc::ServiceOptions::Norm::Lp);
            CHECK_THAT(pNorm, Catch::Matchers::WithinRel(1.1, 1.e-6));
            CHECK_THAT(optionsCopy.getMaximumSearchDepth(),
                       Catch::Matchers::WithinRel(50000, 1.e-4));
            CHECK_THAT(optionsCopy.getHorizontalRefinement(),
                       Catch::Matchers::WithinRel(62000, 1.e-4));
            CHECK_THAT(optionsCopy.getOriginTimeSearchWindow(),
                       Catch::Matchers::WithinRel(53, 1.e-8));
            CHECK(optionsCopy.getNumberOfFunctionEvaluations() == 932);
            CHECK(optionsCopy.getNumberOfParticles() == 83);
            CHECK(optionsCopy.getNumberOfGenerations() == 12);
            CHECK(optionsCopy.getReceiveHighWaterMark() == 500);
            CHECK(optionsCopy.getSendHighWaterMark() == 1000);
        }
    }
}

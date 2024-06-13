#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
//#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
//#include "urts/broadcasts/internal/pick/publisherOptions.hpp"
#include "urts/services/scalable/associators/massociate/arrival.hpp"
#include "urts/services/scalable/associators/massociate/origin.hpp"
#include "urts/services/scalable/associators/massociate/pick.hpp"
#include "urts/services/scalable/associators/massociate/associationRequest.hpp"
#include "urts/services/scalable/associators/massociate/associationResponse.hpp"
#include "urts/services/scalable/associators/massociate/serviceOptions.hpp"
#include "urts/services/scalable/associators/massociate/service.hpp"
#include "urts/services/scalable/associators/massociate/requestorOptions.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

namespace UMASS = URTS::Services::Scalable::Associators::MAssociate;

namespace
{

bool equals(const UMASS::Arrival &lhs, const UMASS::Arrival &rhs)
{
    if (lhs.getNetwork() != rhs.getNetwork()){return false;}
    if (lhs.getStation() != rhs.getStation()){return false;}
    if (lhs.getChannel() != rhs.getChannel()){return false;}
    if (lhs.getLocationCode() != rhs.getLocationCode())
    {
        return false;
    }
    if (lhs.getPhase() != rhs.getPhase()){return false;}
    if (lhs.getTime() != rhs.getTime()){return false;}
    if (lhs.getIdentifier() != rhs.getIdentifier()){return false;}
    if (lhs.getTravelTime())
    {
        if (rhs.getTravelTime())
        {
            if (std::abs(*lhs.getTravelTime() - *rhs.getTravelTime()) > 1.e-7)
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (rhs.getTravelTime()){return false;}
    }
    return true;
}

bool equals(const UMASS::Pick &lhs, const UMASS::Pick &rhs)
{
    if (lhs.getNetwork() != rhs.getNetwork()){return false;}
    if (lhs.getStation() != rhs.getStation()){return false;}
    if (lhs.getChannel() != rhs.getChannel()){return false;}
    if (lhs.getLocationCode() != rhs.getLocationCode())
    {   
        return false;
    }   
    if (lhs.getPhaseHint() != rhs.getPhaseHint()){return false;}
    if (lhs.getTime() != rhs.getTime()){return false;}
    if (lhs.getIdentifier() != rhs.getIdentifier()){return false;}
    if (std::abs(lhs.getStandardError() 
               - rhs.getStandardError()) > 1.e-10){return false;}
    return true;
}

bool equals(const UMASS::Origin &lhs, const UMASS::Origin &rhs)
{
    if (lhs.getTime() != rhs.getTime()){return false;}
    if (std::abs(lhs.getLatitude() - rhs.getLatitude()) > 1.e-10)
    {
        return false;
    }
    if (std::abs(lhs.getLongitude() - rhs.getLongitude()) > 1.e-10)
    {
        return false;
    }
    if (std::abs(lhs.getDepth() - rhs.getDepth()) > 1.e-10)
    {
        return false;
    }
    auto arrivals1 = lhs.getArrivals();
    auto arrivals2 = rhs.getArrivalsReference();
    if (arrivals1.size() != arrivals2.size()){return false;}
    auto nArrivals = static_cast<int> (arrivals1.size());
    for (int i = 0; i < nArrivals; ++i)
    {
        bool matched{false};
        for (int j = 0; j < nArrivals; ++j)
        {
            if (::equals(arrivals1.at(i), arrivals2.at(j)))
            {
                matched = true;
                break;
            }
        }
        if (!matched){return false;}
    }
    return true;
}

UMASS::Arrival createArrival(const std::string &network,
                             const std::string &station,
                             const std::string &channel,
                             const std::string &locationCode,
                             const std::string &phase,
                             const int64_t identifier,
                             const double time,
                             const double travelTime)
{
    UMASS::Arrival arrival;
    arrival.setNetwork(network);
    arrival.setStation(station);
    arrival.setChannel(channel);
    arrival.setLocationCode(locationCode);
    if (phase == "P")
    {
        arrival.setPhase(UMASS::Arrival::Phase::P);
    }
    else
    {
        arrival.setPhase(UMASS::Arrival::Phase::S);
    }
    arrival.setTime(time);
    arrival.setTravelTime(travelTime);
    arrival.setIdentifier(identifier);
    return arrival;
}

UMASS::Pick createPick(const std::string &network,
                       const std::string &station,
                       const std::string &channel,
                       const std::string &locationCode,
                       const std::string &phaseHint,
                       const int64_t identifier,
                       const double time,
                       const double standardError)
{
    UMASS::Pick pick;
    pick.setNetwork(network);
    pick.setStation(station);
    pick.setChannel(channel);
    pick.setLocationCode(locationCode);
    if (phaseHint == "P")
    {   
        pick.setPhaseHint(UMASS::Pick::PhaseHint::P);
    }   
    else
    {   
        pick.setPhaseHint(UMASS::Pick::PhaseHint::S);
    }   
    pick.setTime(time);
    pick.setStandardError(standardError);
    pick.setIdentifier(identifier);
    return pick;
}

UMASS::Origin createBogusOrigin1()
{
    UMASS::Origin origin;
    std::vector<UMASS::Arrival> arrivals;
    arrivals.push_back( ::createArrival("UU", "LTU", "EHZ", "02", "P", 1, 1714951814 + 3.2, 3.2) );  
    arrivals.push_back( ::createArrival("UU", "ETW", "ENN", "01", "P", 2, 1714951814 + 5.1, 5.1) );
    arrivals.push_back( ::createArrival("UU", "LTU", "ENN", "01", "S", 3, 1714951814 + 5.5, 5.5) );    
    arrivals.push_back( ::createArrival("UU", "SPU", "HHZ", "01", "P", 4, 1714951814 + 5.8, 5.8) );
    arrivals.push_back( ::createArrival("UU", "SPU", "HHN", "01", "S", 5, 1714951814 +10.1,10.1) );
 
    origin.setArrivals(arrivals);
    origin.setLatitude(41);
    origin.setLongitude(-112);
    origin.setDepth(8600);
    origin.setTime(1714951814);
    return origin;
}

UMASS::Origin createBogusOrigin2()
{   
    UMASS::Origin origin;
    std::vector<UMASS::Arrival> arrivals;
    arrivals.push_back( ::createArrival("UU", "LTU", "EHZ", "02", "P", 1, 1714951884 + 3.2, 3.2) );
    arrivals.push_back( ::createArrival("UU", "ETW", "ENN", "01", "P", 2, 1714951884 + 5.1, 5.1) );    
    arrivals.push_back( ::createArrival("UU", "LTU", "ENN", "01", "S", 3, 1714951884 + 5.5, 5.5) );
    arrivals.push_back( ::createArrival("UU", "SPU", "HHZ", "01", "P", 4, 1714951884 + 5.8, 5.8) );
    arrivals.push_back( ::createArrival("UU", "SPU", "HHN", "01", "S", 5, 1714951884 +10.1,10.1) );

    origin.setArrivals(arrivals);
    origin.setLatitude(40);
    origin.setLongitude(-111);
    origin.setDepth(8680);
    origin.setTime(1714951884);
    return origin;
}
          
}

TEST_CASE("URTS::Services::Scalable::Associators::MAssociate", "[models]")
{
    SECTION("pick")
    {
        constexpr double pickTime{102};
        constexpr std::chrono::microseconds pickTimeMuS{102000000};
        const int64_t identifier{2323};
        UMASS::Pick pick;
        REQUIRE_NOTHROW(pick.setNetwork("UU"));
        REQUIRE_NOTHROW(pick.setStation("FSU"));
        REQUIRE_NOTHROW(pick.setChannel("HHE"));
        REQUIRE_NOTHROW(pick.setLocationCode("01"));
        pick.setPhaseHint(UMASS::Pick::PhaseHint::S);    
        pick.setTime(pickTime);
        pick.setIdentifier(identifier);
        pick.setStandardError(0.1);

        REQUIRE(pick.getNetwork() == "UU");
        REQUIRE(pick.getStation() == "FSU");
        REQUIRE(pick.getChannel() == "HHE");
        REQUIRE(pick.getLocationCode() == "01");
        REQUIRE(pick.getPhaseHint() == UMASS::Pick::PhaseHint::S);
        REQUIRE(pick.getTime() == pickTimeMuS);
        REQUIRE(pick.getIdentifier() == identifier);
        REQUIRE(std::abs(pick.getStandardError() - 0.1) < 1.e-14);
    }

    SECTION("pick from broadcast pick")
    {
        constexpr double pickTime{103};
        constexpr std::chrono::microseconds pickTimeMuS{103000000};
        const int64_t identifier{2324};
        URTS::Broadcasts::Internal::Pick::Pick uPick;
        uPick.setNetwork("WY");
        uPick.setStation("YJC");
        uPick.setChannel("HHZ");
        uPick.setLocationCode("01");
        uPick.setPhaseHint("P");
        uPick.setTime(pickTime);
        uPick.setIdentifier(identifier);
        URTS::Broadcasts::Internal::Pick::UncertaintyBound lowerBound;
        // Put picks at +/- 2 standard deviations
        lowerBound.setPercentile(2.28); //15.87);
        lowerBound.setPerturbation(std::chrono::microseconds {-200000});
        URTS::Broadcasts::Internal::Pick::UncertaintyBound upperBound;
        upperBound.setPercentile(97.72); //84.13);
        upperBound.setPerturbation(std::chrono::microseconds {+200000});
        uPick.setLowerAndUpperUncertaintyBound( {lowerBound, upperBound} );

        UMASS::Pick pick{uPick};
        REQUIRE(pick.getNetwork() == "WY");
        REQUIRE(pick.getStation() == "YJC");
        REQUIRE(pick.getChannel() == "HHZ");
        REQUIRE(pick.getLocationCode() == "01");
        REQUIRE(pick.getPhaseHint() == UMASS::Pick::PhaseHint::P);
        REQUIRE(pick.getTime() == pickTimeMuS);
        REQUIRE(pick.getIdentifier() == identifier); 
        // Should be half of 0.2 seconds
        REQUIRE( (pick.getStandardError() - 0.1) < 1.e-4 );
    }

    SECTION("arrival")
    {
        UMASS::Arrival arrival;
        arrival.setNetwork("US");
        arrival.setStation("DUG");
        arrival.setChannel("BHZ");
        arrival.setLocationCode("00");
        arrival.setPhase(UMASS::Arrival::Phase::P);
        arrival.setTime(std::chrono::microseconds {930218});
        arrival.setIdentifier(32212);
        arrival.setTravelTime(12.1);

        REQUIRE(arrival.getNetwork() == "US");
        REQUIRE(arrival.getStation() == "DUG");
        REQUIRE(arrival.getChannel() == "BHZ");
        REQUIRE(arrival.getLocationCode() == "00");
        REQUIRE(arrival.getTime() == std::chrono::microseconds {930218});
        REQUIRE(arrival.getIdentifier() == 32212);
        REQUIRE(std::abs(*arrival.getTravelTime() - 12.1) < 1.e-14);
    }

    SECTION("origin")
    {
        UMASS::Origin origin;
        REQUIRE_NOTHROW(origin.setLatitude(41.));
        origin.setLongitude(-112);
        REQUIRE_NOTHROW(origin.setDepth(8600));
        origin.setTime(std::chrono::microseconds {1717710340200000});

        std::vector<UMASS::Arrival> arrivals;
        arrivals.push_back( ::createArrival("UU", "LTU", "EHZ", "02", "P", 1, 1714951814 + 3.2, 3.2) ); 
        arrivals.push_back( ::createArrival("UU", "ETW", "ENN", "01", "P", 2, 1714951814 + 5.1, 5.1) );
        arrivals.push_back( ::createArrival("UU", "LTU", "ENN", "01", "S", 3, 1714951814 + 5.5, 5.5) );        
        arrivals.push_back( ::createArrival("UU", "SPU", "HHZ", "01", "P", 4, 1714951814 + 5.8, 5.8) );
        arrivals.push_back( ::createArrival("UU", "SPU", "HHN", "01", "S", 5, 1714951814 +10.1,10.1) );
        origin.setArrivals(arrivals);
        
        REQUIRE(std::abs(origin.getLatitude() - 41) < 1.e-10);
        REQUIRE(std::abs(origin.getLongitude() - -112) < 1.e-10);
        REQUIRE(std::abs(origin.getDepth() - 8600) < 1.e-10);
        REQUIRE(origin.getTime() == std::chrono::microseconds {1717710340200000});
        auto arrivalsBack = origin.getArrivals();
        REQUIRE(arrivalsBack.size() == arrivals.size());
        for (size_t i = 0; i < arrivalsBack.size(); ++i)
        {
            REQUIRE(::equals(arrivalsBack.at(i), arrivals.at(i)));
        }
    }
}

TEST_CASE("URTS::Services::Scalable::Associators::MAssociate", "[messages]")
{
    SECTION("association request")
    {
        std::vector<UMASS::Pick> picks;
        picks.push_back( ::createPick("UU", "LTU", "EHZ", "02", "P", 1, 1714951814 + 3.2, 0.1) );  
        picks.push_back( ::createPick("UU", "ETW", "ENN", "01", "P", 2, 1714951814 + 5.1, 0.1) );
        picks.push_back( ::createPick("UU", "LTU", "ENN", "01", "S", 3, 1714951814 + 5.5, 0.2) );    
        picks.push_back( ::createPick("UU", "SPU", "HHZ", "01", "P", 4, 1714951814 + 5.8, 0.1) );
        picks.push_back( ::createPick("UU", "SPU", "HHN", "01", "S", 5, 1714951814 +10.1, 0.2) );

        int64_t identifier{64};
        UMASS::AssociationRequest request;
        request.setIdentifier(identifier);
        request.setPicks(picks);
        auto message = request.toMessage();

        UMASS::AssociationRequest requestFromMessage;
        requestFromMessage.fromMessage(message);
        REQUIRE(requestFromMessage.getIdentifier() == identifier);
        auto picksBack = requestFromMessage.getPicks();
        REQUIRE(picksBack.size() == picks.size());
        for (size_t i = 0; i < picks.size(); ++i)
        {
            bool matched{false};
            for (size_t j = 0; j < picksBack.size(); ++j)
            {
                if (::equals(picks.at(i), picksBack.at(j)))
                {
                    matched = true;
                    break;
                }
            }
            REQUIRE(matched);
       }
    }

    SECTION("association response")
    {

        std::vector<UMASS::Pick> picks;
        picks.push_back( ::createPick("UU", "LTU", "EHZ", "02", "P", 1, 1714951814 + 3.2, 0.1) );  
        picks.push_back( ::createPick("UU", "ETW", "ENN", "01", "P", 2, 1714951814 + 5.1, 0.1) );
        picks.push_back( ::createPick("UU", "LTU", "ENN", "01", "S", 3, 1714951814 + 5.5, 0.2) );    
        picks.push_back( ::createPick("UU", "SPU", "HHZ", "01", "P", 4, 1714951814 + 5.8, 0.1) );
        picks.push_back( ::createPick("UU", "SPU", "HHN", "01", "S", 5, 1714951814 +10.1, 0.2) );

        int64_t identifier{65};
        UMASS::AssociationResponse::ReturnCode returnCode{
            UMASS::AssociationResponse::ReturnCode::Success};
        UMASS::AssociationResponse response;
        response.setIdentifier(identifier);
        response.setUnassociatedPicks(picks);
        response.setReturnCode(returnCode);
        std::vector<UMASS::Origin> origins{createBogusOrigin1(),
                                           createBogusOrigin2()};
        response.setOrigins(origins);

        auto message = response.toMessage();

        UMASS::AssociationResponse responseFromMessage;
        responseFromMessage.fromMessage(message);
        REQUIRE(responseFromMessage.getIdentifier() == identifier);
        REQUIRE(responseFromMessage.getReturnCode() == returnCode);
        auto originsBack = response.getOrigins();
        REQUIRE(originsBack.size() == origins.size());
        auto nOrigins = static_cast<int> (origins.size());        
        for (int i = 0; i < nOrigins; ++i)
        {
            bool matched = false;
            for (int j = 0; j < nOrigins; ++j)
            {
                if (::equals(origins.at(i), originsBack.at(j)))
                {
                    matched = true;
                    break;
                }
            } 
            REQUIRE(matched);
        }
    }
}

TEST_CASE("URTS::Services::Scalable::Associators::MAssociate", "[ServiceOptions]")
{
    double dbscanEpsilon{0.30};
    int dbscanMinimumClusterSize{5};
    double maximumDistance{120000};
    std::pair<double, double> searchDepths{-1000, 15000};
    UMASS::ServiceOptions serviceOptions;  
    serviceOptions.setRegion(UMASS::ServiceOptions::Region::Utah);
    serviceOptions.setDBSCANEpsilon(dbscanEpsilon);
    serviceOptions.setDBSCANMinimumClusterSize(dbscanMinimumClusterSize);
    serviceOptions.setMaximumDistanceToAssociate(maximumDistance);
    serviceOptions.setExtentInDepth(searchDepths);
    serviceOptions.setReceiveHighWaterMark(500);
    serviceOptions.setSendHighWaterMark(1000);
    SECTION("service options copy")
    {
        UMASS::ServiceOptions optionsCopy{serviceOptions}; 
        REQUIRE(optionsCopy.getRegion() == UMASS::ServiceOptions::Region::Utah);
        REQUIRE(std::abs(optionsCopy.getDBSCANEpsilon() - dbscanEpsilon)
                       < 1.e-10);
        REQUIRE(optionsCopy.getDBSCANMinimumClusterSize() ==
                dbscanMinimumClusterSize);
        REQUIRE(std::abs(optionsCopy.getMaximumDistanceToAssociate()
                       - maximumDistance) < 1.e-8);
        REQUIRE(std::abs(optionsCopy.getExtentInDepth().first
                       - searchDepths.first) < 1.e-8);
        REQUIRE(std::abs(optionsCopy.getExtentInDepth().second
                       - searchDepths.second) < 1.e-8);
        REQUIRE(optionsCopy.getReceiveHighWaterMark() == 500);
        REQUIRE(optionsCopy.getSendHighWaterMark() == 1000);
    }
}

TEST_CASE("URTS::Services::Scalable::Associators::MAssociate", "[RequestorOptions]")
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int sendHWM{105};
    const int recvHWM{106};
    const std::chrono::milliseconds sendTimeOut{120};
    const std::chrono::milliseconds recvTimeOut{145};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    UMASS::RequestorOptions options;
    REQUIRE_NOTHROW(options.setAddress(address));
    REQUIRE_NOTHROW(options.setSendHighWaterMark(sendHWM));
    REQUIRE_NOTHROW(options.setReceiveHighWaterMark(recvHWM));
    options.setSendTimeOut(sendTimeOut);
    options.setReceiveTimeOut(recvTimeOut);
    options.setZAPOptions(zapOptions);

    SECTION("from copy constructor")
    {
        UMASS::RequestorOptions copy(options);
        REQUIRE(copy.getAddress() == address);
        REQUIRE(copy.getSendHighWaterMark() == sendHWM);
        REQUIRE(copy.getReceiveHighWaterMark() == recvHWM);
        REQUIRE(copy.getSendTimeOut() == sendTimeOut); 
        REQUIRE(copy.getReceiveTimeOut() == recvTimeOut);
        REQUIRE(copy.getZAPOptions().getSecurityLevel() ==
                zapOptions.getSecurityLevel());
    }

    SECTION("clear and check defaults")
    {
        options.clear();
        REQUIRE(options.getSendHighWaterMark() == 2048);
        REQUIRE(options.getReceiveHighWaterMark() == 0);
        REQUIRE(options.getSendTimeOut() ==
                std::chrono::milliseconds {0});
        REQUIRE(options.getReceiveTimeOut() ==
                std::chrono::milliseconds {60000});
        zapOptions.setGrasslandsClient();
        REQUIRE(options.getZAPOptions().getSecurityLevel() ==
                zapOptions.getSecurityLevel());
    }
}


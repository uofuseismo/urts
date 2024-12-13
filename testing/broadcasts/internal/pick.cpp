#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
#include "urts/broadcasts/internal/pick/publisherOptions.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::Pick"

using namespace URTS::Broadcasts::Internal::Pick;

TEST_CASE("URTS::Broadcasts::Internal::Pick", "[uncertainty_bound]")
{
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
}

TEST_CASE("URTS::Broadcasts::Internal::Pick", "[pick]")
{
    Pick pick;
    const uint64_t pickID{84823};
    const std::string network{"UU"};
    const std::string station{"MOUT"};
    const std::string channel{"EHP"};
    const std::vector<std::string> originalChannels{"EHZ", "EHN", "EHE"};
    const std::string locationCode{"01"};
    const std::string phaseHint{"P"};
    const std::vector<std::string> algorithms{"thresholdPick", "mlPicker"};
    const double time{500};
    const double snr{13};
    const std::chrono::microseconds timeRef{static_cast<int64_t> (time*1.e6)};
    const Pick::FirstMotion fm{Pick::FirstMotion::Up};
    const Pick::ReviewStatus reviewStatus{Pick::ReviewStatus::Manual};

    UncertaintyBound lowerBoundRef;
    const double lowerPercentile{5};
    const std::chrono::microseconds lowerPerturbation{-1500};
    lowerBoundRef.setPercentile(lowerPercentile);
    lowerBoundRef.setPerturbation(lowerPerturbation);

    UncertaintyBound upperBoundRef;
    const double upperPercentile{95};
    const std::chrono::microseconds upperPerturbation{1500};
    upperBoundRef.setPercentile(upperPercentile);
    upperBoundRef.setPerturbation(upperPerturbation);

    pick.setIdentifier(pickID);
    pick.setTime(time);
    REQUIRE_NOTHROW(pick.setNetwork(network));
    REQUIRE_NOTHROW(pick.setStation(station));
    REQUIRE_NOTHROW(pick.setChannel(channel));
    REQUIRE_NOTHROW(pick.setLocationCode(locationCode));
    REQUIRE(pick.getMessageType() == MESSAGE_TYPE);
    REQUIRE_NOTHROW(pick.setLowerAndUpperUncertaintyBound(
                    std::pair {lowerBoundRef, upperBoundRef}));
    pick.setFirstMotion(fm);
    pick.setSignalToNoiseRatio(snr);
    pick.setReviewStatus(reviewStatus);
    pick.setPhaseHint(phaseHint);
    pick.setProcessingAlgorithms(algorithms);
    pick.setOriginalChannels(originalChannels);

    SECTION("pick deep copy")
    {
        Pick copy(pick);
        auto pickMessage = pick.toMessage();
        REQUIRE_NOTHROW(copy.fromMessage(pickMessage));

        REQUIRE(copy.getIdentifier() == pickID);
        REQUIRE(copy.getTime() == timeRef);
        REQUIRE(copy.getNetwork() == network);
        REQUIRE(copy.getStation() == station);
        REQUIRE(copy.getChannel() == channel);
        REQUIRE(copy.getLocationCode() == locationCode);
        REQUIRE(copy.getFirstMotion() == fm);
        REQUIRE(copy.getReviewStatus() == reviewStatus);
        REQUIRE(copy.getPhaseHint() == phaseHint);
        REQUIRE(std::abs(copy.getSignalToNoiseRatio() - snr) < 1.e-14);
        auto algorithmsBack = copy.getProcessingAlgorithms();
        REQUIRE(algorithms.size() == algorithmsBack.size());
        for (int i = 0; i < static_cast<int> (algorithms.size()); ++i)
        {
            REQUIRE(algorithms[i] == algorithmsBack[i]);
        }
        auto originalChannelsBack = copy.getOriginalChannels();
        REQUIRE(originalChannels.size() == originalChannelsBack.size());
        for (int i = 0; i < static_cast<int> (originalChannels.size()); ++i)
        {
            REQUIRE(originalChannels[i] == originalChannelsBack[i]);
        }
 
        auto [lowerBound, upperBound ]
             = copy.getLowerAndUpperUncertaintyBound();
        REQUIRE(std::abs(lowerBoundRef.getPercentile()
                       - lowerBound.getPercentile()) < 1.e-14);
        REQUIRE(lowerBoundRef.getPerturbation() ==
                lowerBound.getPerturbation());
        REQUIRE(std::abs(upperBoundRef.getPercentile()
                       - upperBound.getPercentile()) <1.e-14);
        REQUIRE(upperBoundRef.getPerturbation() ==
                upperBound.getPerturbation());
    }
}

TEST_CASE("URTS::Broadcasts::Internal::Pick", "[subscriber_options]")
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
        REQUIRE(options.getHighWaterMark() == 0);
        REQUIRE(options.getTimeOut() == std::chrono::milliseconds {10});
        REQUIRE(options.getZAPOptions().getSecurityLevel() ==
                UMPS::Authentication::SecurityLevel::Grasslands);
    }
}

TEST_CASE("URTS::Broadcasts::Internal::Pick", "[publisher_options]")
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
        REQUIRE(options.getHighWaterMark() == 0);
        REQUIRE(options.getTimeOut() == std::chrono::milliseconds {1000});
        REQUIRE(options.getZAPOptions().getSecurityLevel() ==
                UMPS::Authentication::SecurityLevel::Grasslands);
    }
}


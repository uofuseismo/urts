#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
#include "urts/broadcasts/internal/pick/publisherOptions.hpp"
#include <gtest/gtest.h>
namespace
{

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::Pick"

using namespace URTS::Broadcasts::Internal::Pick;

TEST(BroadcastsInternalPick, UncertaintyBound)
{
    const double percentile{95};
    const std::chrono::microseconds perturbation{1500};

    UncertaintyBound bound;
    EXPECT_NO_THROW(bound.setPercentile(percentile));
    bound.setPerturbation(perturbation);

    UncertaintyBound copy(bound);
    EXPECT_NEAR(bound.getPercentile(), percentile, 1.e-14);
    EXPECT_EQ(bound.getPerturbation(), perturbation);
 
    bound.clear();
    EXPECT_NEAR(bound.getPercentile(), 50, 1.e-14);
    EXPECT_EQ(bound.getPerturbation(), std::chrono::microseconds {0});
}


TEST(BroadcastsInternalPick, Pick)
{
    Pick pick;
    const uint64_t pickID{84823};
    const std::string network{"UU"};
    const std::string station{"MOUT"};
    const std::string channel{"EHZ"};
    const std::string locationCode{"01"};
    const std::string phaseHint{"P"};
    const std::vector<std::string> algorithms{"thresholdPick", "mlPicker"};
    const double time = 500;
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
    EXPECT_NO_THROW(pick.setNetwork(network));
    EXPECT_NO_THROW(pick.setStation(station));
    EXPECT_NO_THROW(pick.setChannel(channel));
    EXPECT_NO_THROW(pick.setLocationCode(locationCode));
    EXPECT_EQ(pick.getMessageType(), MESSAGE_TYPE);
    EXPECT_NO_THROW(pick.setLowerAndUpperUncertaintyBound(
                    std::pair {lowerBoundRef, upperBoundRef}));
    pick.setFirstMotion(fm);
    pick.setReviewStatus(reviewStatus);
    pick.setPhaseHint(phaseHint);
    pick.setProcessingAlgorithms(algorithms);

    Pick copy(pick);
    auto pickMessage = pick.toMessage();
    EXPECT_NO_THROW(copy.fromMessage(pickMessage));

    EXPECT_EQ(copy.getIdentifier(), pickID);
    EXPECT_EQ(copy.getTime(), timeRef);
    EXPECT_EQ(copy.getNetwork(), network);
    EXPECT_EQ(copy.getStation(), station);
    EXPECT_EQ(copy.getChannel(), channel);
    EXPECT_EQ(copy.getLocationCode(), locationCode);
    EXPECT_EQ(copy.getFirstMotion(), fm);
    EXPECT_EQ(copy.getReviewStatus(), reviewStatus);
    EXPECT_EQ(copy.getPhaseHint(), phaseHint);
    auto algorithmsBack = copy.getProcessingAlgorithms();
    EXPECT_EQ(algorithms.size(), algorithmsBack.size());
    for (int i = 0; i < static_cast<int> (algorithms.size()); ++i)
    {
        EXPECT_EQ(algorithms[i], algorithmsBack[i]);
    }
    auto [lowerBound, upperBound ] = copy.getLowerAndUpperUncertaintyBound();
    EXPECT_NEAR(lowerBoundRef.getPercentile(), lowerBound.getPercentile(),
                1.e-14);
    EXPECT_EQ(lowerBoundRef.getPerturbation(), lowerBound.getPerturbation()); 
    EXPECT_NEAR(upperBoundRef.getPercentile(), upperBound.getPercentile(),
                1.e-14);
    EXPECT_EQ(upperBoundRef.getPerturbation(), upperBound.getPerturbation());
}

TEST(BroadcastsInternalDataPacket, SubscriberOptions)
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int recvHWM{106};
    const std::chrono::milliseconds recvTimeOut{145};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    SubscriberOptions options;
    EXPECT_NO_THROW(options.setAddress(address));
    EXPECT_NO_THROW(options.setHighWaterMark(recvHWM));
    EXPECT_NO_THROW(options.setTimeOut(recvTimeOut));
    EXPECT_NO_THROW(options.setZAPOptions(zapOptions));

    SubscriberOptions copy(options);
    EXPECT_EQ(options.getAddress(), address);
    EXPECT_EQ(options.getHighWaterMark(), recvHWM);
    EXPECT_EQ(options.getTimeOut(), recvTimeOut);
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());    

    options.clear();
    EXPECT_FALSE(options.haveAddress());
    EXPECT_EQ(options.getHighWaterMark(), 0);
    EXPECT_EQ(options.getTimeOut(), std::chrono::milliseconds {10});
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              UMPS::Authentication::SecurityLevel::Grasslands);
}

TEST(BroadcastsInternalDataPacket, PublisherOptions)
{
    PublisherOptions options;
    const std::string address{"tcp://127.0.0.1:8080"};
    const int sendHWM{113};
    const std::chrono::milliseconds sendTimeOut{155};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    EXPECT_NO_THROW(options.setAddress(address));
    EXPECT_NO_THROW(options.setHighWaterMark(sendHWM));
    EXPECT_NO_THROW(options.setTimeOut(sendTimeOut));
    EXPECT_NO_THROW(options.setZAPOptions(zapOptions));

    PublisherOptions copy(options);
    EXPECT_EQ(options.getAddress(), address);
    EXPECT_EQ(options.getHighWaterMark(), sendHWM);
    EXPECT_EQ(options.getTimeOut(), sendTimeOut);
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());    

    options.clear();
    EXPECT_FALSE(options.haveAddress());
    EXPECT_EQ(options.getHighWaterMark(), 0);
    EXPECT_EQ(options.getTimeOut(), std::chrono::milliseconds {1000});
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              UMPS::Authentication::SecurityLevel::Grasslands);
}

}

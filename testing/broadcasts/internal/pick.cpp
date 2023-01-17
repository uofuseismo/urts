#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
//#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
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
    const std::string algorithm{"autoPicker"};
    const double time = 500;
    const std::chrono::microseconds timeRef{static_cast<int64_t> (time*1.e6)};
    const Pick::Polarity polarity{Pick::Polarity::Up};
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
    pick.setPolarity(polarity);
    pick.setReviewStatus(reviewStatus);
    pick.setPhaseHint(phaseHint);
    pick.setAlgorithm(algorithm);

    Pick copy(pick);
    auto pickMessage = pick.toMessage();
    EXPECT_NO_THROW(copy.fromMessage(pickMessage));

    EXPECT_EQ(copy.getIdentifier(), pickID);
    EXPECT_EQ(copy.getTime(), timeRef);
    EXPECT_EQ(copy.getNetwork(), network);
    EXPECT_EQ(copy.getStation(), station);
    EXPECT_EQ(copy.getChannel(), channel);
    EXPECT_EQ(copy.getLocationCode(), locationCode);
    EXPECT_EQ(copy.getPolarity(), polarity);
    EXPECT_EQ(copy.getReviewStatus(), reviewStatus);
    EXPECT_EQ(copy.getPhaseHint(), phaseHint);
    EXPECT_EQ(copy.getAlgorithm(), algorithm);
    auto [lowerBound, upperBound ] = copy.getLowerAndUpperUncertaintyBound();
    EXPECT_NEAR(lowerBoundRef.getPercentile(), lowerBound.getPercentile(),
                1.e-14);
    EXPECT_EQ(lowerBoundRef.getPerturbation(), lowerBound.getPerturbation()); 
    EXPECT_NEAR(upperBoundRef.getPercentile(), upperBound.getPercentile(),
                1.e-14);
    EXPECT_EQ(upperBoundRef.getPerturbation(), upperBound.getPerturbation());
}

}

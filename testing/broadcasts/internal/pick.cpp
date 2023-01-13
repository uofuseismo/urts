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

}

}

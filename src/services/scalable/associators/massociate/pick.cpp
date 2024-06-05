#include <cmath>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/math/special_functions/erf.hpp>
#include "urts/services/scalable/associators/massociate/pick.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "private/isEmpty.hpp"

using namespace URTS::Services::Scalable::Associators::MAssociate;

namespace
{

/// @brief This is a convenience function that transforms a perturbation 
///        tabulated at a given percentile of a Gaussian distribution
///        to it's perturbation at 1 standard deviation if the probability
///        is greater than 0.5 or -1 standard deviation if the probability
///        is less than 0.5.
double transformPerturbation(const double perturbation,
                             const double probability)
{
    //const double sqrt2 = std::sqrt(2.0);
    double perturbationAtOneStandardError{perturbation};
    if (probability <= 0 || probability >= 1)
    {
        throw std::invalid_argument("Probability must be in range (0,1)");
    }
    if (std::abs(0.5 - probability) < 1.e-6)
    {
        throw std::invalid_argument("Cannot work with P = 0.5");
    }
    if (probability > 0.5)
    {
        // 1: P(X < L) = 1/2 + 1/2 erf( (L - mu)/(sqrt(2)*sigma) )
        // 2: mu = 0 and sigma = 1
        // 3: P(X < L) = 1/2 + 1/2 erf( L/sqrt(2) )
        // Note, L is the z-score.  Hence, given a probability we compute:
        // 4: L = sqrt(2)*erf^{-1}( 2*P(X < L) - 1 )
        double z = std::sqrt(2.0)*boost::math::erf_inv<double> (2*probability - 1); 
        // Given the z-score at this probability we now scale it to 1 standard
        // deviation - i.e., p/z = p1/1 -> p1 = p/z. 
        perturbationAtOneStandardError = perturbation/z;
    }
    else if (probability < 0.5)
    {
        // Same logic as before, but the convention is we add the perturbation
        // so this will likely be a negative number and to get the signs to
        // work out we introduce a negative
        double z = std::sqrt(2.0)*boost::math::erf_inv<double> (2*probability - 1);
        perturbationAtOneStandardError =-perturbation/z;
    } 
    return perturbationAtOneStandardError;
}

/// @brief Attempts to create a standard error at one standard deviation for
///        errors at the lower and upper percentils assuming an underlying
///        slightly non-symmetric Gaussian'ish distribution by averaging
///        the rescaled perturbation at +/- standard error. 
double transformPerturbation(
    const double lowerPerturbation, const double lowerProbability,
    const double upperPerturbation, const double upperProbability)
{
    if (lowerPerturbation >= 0)
    {
        throw std::invalid_argument("Lower perturbation should be negative");
    }
    if (upperPerturbation <= 0)
    {
        throw std::invalid_argument("Upper perturbation should be positive");
    }
    auto lowerPerturbationAtOneStandardError
         = ::transformPerturbation(lowerPerturbation, lowerProbability);
    auto upperPerturbationAtOneStandardError
        = ::transformPerturbation(upperPerturbation, upperProbability);
    return 0.5*(std::abs(lowerPerturbationAtOneStandardError)
              + std::abs(upperPerturbationAtOneStandardError));
} 

}

class Pick::PickImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::chrono::microseconds mTime{0};
    double mStandardError{0.1};
    uint64_t mIdentifier{0};
    PhaseHint mPhaseHint;
    bool mHaveTime{false};
    bool mHaveIdentifier{false};
    bool mHavePhaseHint{false};
};

/// Constructor
Pick::Pick() :
    pImpl(std::make_unique<PickImpl> ())
{
}

/// Copy constructor
Pick::Pick(const Pick &pick)
{
    *this = pick;
}

/// Construct from a pick
Pick::Pick(const URTS::Broadcasts::Internal::Pick::Pick &pick) :
    pImpl(std::make_unique<PickImpl> ())
{
    if (!pick.haveNetwork())
    {
        throw std::invalid_argument("Network not set");
    }
    if (!pick.haveStation())
    {
        throw std::invalid_argument("Station not set");
    }
    if (!pick.haveChannel())
    {
        throw std::invalid_argument("Channel not set");
    }
    if (!pick.haveLocationCode())
    {
        throw std::invalid_argument("Location code not set");
    }
    if (!pick.haveTime())
    {
        throw std::invalid_argument("Time not set");
    }
    auto phaseHint = pick.getPhaseHint();
    bool guessPhaseHint{false};
    if (phaseHint.empty())
    {
        guessPhaseHint = true;
    }
    else
    {
        if (phaseHint != "P" && phaseHint != "S")
        {
            guessPhaseHint = true;
        }
    }
    if (guessPhaseHint)
    {
        auto channel = pick.getChannel();
        if (channel.back() == 'Z' || channel.back() == 'P')
        {
            phaseHint = "P";
        }
        else if (channel.back() == 'N' || channel.back() == '1' ||
                 channel.back() == 'E' || channel.back() == '2' ||
                 channel.back() == 'S')
        {
            phaseHint = "S";
        }
        else
        {
            throw std::invalid_argument("Could not guess phase hint");
        }
    } 
    // Standard error
    double standardError{0.1};
    if (pick.haveLowerAndUpperUncertaintyBound())
    {
        auto bounds = pick.getLowerAndUpperUncertaintyBound();
        auto lowerProbability = bounds.first.getPercentile()/100.0;
        auto lowerPerturbation = bounds.first.getPerturbation().count()*1.e-6;
        auto upperProbability = bounds.second.getPercentile()/100.0;
        auto upperPerturbation = bounds.second.getPerturbation().count()*1.e-6;
        standardError
            = ::transformPerturbation(lowerPerturbation, lowerProbability,
                                      upperPerturbation, upperProbability);
    }
    else
    {
        standardError = 0.1; // P
        if (phaseHint == "S"){standardError = 0.2;}
    }
    // Build the pick
    setNetwork(pick.getNetwork());
    setStation(pick.getStation());
    setChannel(pick.getChannel());
    setLocationCode(pick.getLocationCode());
    setTime(pick.getTime());
    if (phaseHint == "P")
    {
        setPhaseHint(PhaseHint::P);
    }
    else if (phaseHint == "S")
    {
        setPhaseHint(PhaseHint::S);
    }
    else
    {
#ifndef NDEBUG
        assert(false);
#else
        throw std::runtime_error("Algorithm failed to guess phase hint");
#endif
    }
    setIdentifier(pick.getIdentifier());
    setStandardError(standardError);
} 

/// Move constructor
Pick::Pick(Pick &&pick) noexcept
{
    *this = std::move(pick);
}

/// Copy assignment
Pick& Pick::operator=(const Pick &pick)
{
    if (&pick == this){return *this;}
    pImpl = std::make_unique<PickImpl> (*pick.pImpl);
    return *this;
}

/// Move assignment
Pick& Pick::operator=(Pick &&pick) noexcept
{
    if (&pick == this){return *this;}
    pImpl = std::move(pick.pImpl);
    return *this;
}

/// Destructor
Pick::~Pick() = default;

/// Reset the class 
void Pick::clear() noexcept
{
    pImpl = std::make_unique<PickImpl> ();
}

/// Network
void Pick::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Pick::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool Pick::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void Pick::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string Pick::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool Pick::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void Pick::setChannel(const std::string &channel)
{
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string Pick::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set yet");}
    return pImpl->mChannel;
}

bool Pick::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void Pick::setLocationCode(const std::string &location)
{
    if (::isEmpty(location))
    {
        throw std::invalid_argument("Location code is empty");
    }
    pImpl->mLocationCode = location;
}

std::string Pick::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }
    return pImpl->mLocationCode;
}

bool Pick::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Pick time
void Pick::setTime(const double time) noexcept
{
    auto pickTime
       = std::chrono::microseconds{static_cast<int64_t> (std::round(time*1.e6))};
    setTime(pickTime);
}

void Pick::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Pick::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not yet set");}
    return pImpl->mTime;
}

bool Pick::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Pick identifier
void Pick::setIdentifier(const uint64_t id) noexcept
{
    pImpl->mIdentifier = id;
    pImpl->mHaveIdentifier = true;
}

uint64_t Pick::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not yet set");}
    return pImpl->mIdentifier;
}

bool Pick::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// Phase hint
void Pick::setPhaseHint(const PhaseHint phaseHint) noexcept
{
    pImpl->mPhaseHint = phaseHint;
    pImpl->mHavePhaseHint = true;
}

Pick::PhaseHint Pick::getPhaseHint() const
{
    if (!havePhaseHint()){throw std::runtime_error("Phase hint not set");}
    return pImpl->mPhaseHint;
}

bool Pick::havePhaseHint() const noexcept
{
    return pImpl->mHavePhaseHint;
}

/// Uncertainty
void Pick::setStandardError(const double error)
{
    if (error <= 0)
    {
        throw std::invalid_argument("Standard error must be positive");
    }
    pImpl->mStandardError = error;
}

double Pick::getStandardError() const noexcept
{
    return pImpl->mStandardError;
}


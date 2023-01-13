#include <chrono>
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"

using namespace URTS::Broadcasts::Internal::Pick;

class UncertaintyBound::UncertaintyBoundImpl
{
public:
    std::chrono::microseconds mPerturbation{0};
    double mPercentile{50};
};

/// Constructors
UncertaintyBound::UncertaintyBound() :
    pImpl(std::make_unique<UncertaintyBoundImpl> ())
{
}

UncertaintyBound::UncertaintyBound(const UncertaintyBound &bound)
{
    *this = bound;
}

UncertaintyBound::UncertaintyBound(UncertaintyBound &&bound) noexcept
{
    *this = std::move(bound);
}

/// Operators
UncertaintyBound& UncertaintyBound::operator=(const UncertaintyBound &bound)
{
    if (&bound == this){return *this;}
    pImpl = std::make_unique<UncertaintyBoundImpl> (*bound.pImpl);
    return *this;
}

UncertaintyBound& UncertaintyBound::operator=(UncertaintyBound &&bound) noexcept
{
    if (&bound == this){return *this;}
    pImpl = std::move(bound.pImpl);
    return *this;
}

/// Reset class
void UncertaintyBound::clear() noexcept
{
    pImpl = std::make_unique<UncertaintyBoundImpl> ();
}

/// Destructor
UncertaintyBound::~UncertaintyBound() = default;

/// Percentile
void UncertaintyBound::setPercentile(const double percentile)
{
    if (percentile < 0 || percentile > 100)
    { 
        throw std::invalid_argument("Percentile must be in range [0,100]");
    }
    pImpl->mPercentile = percentile;
}

double UncertaintyBound::getPercentile() const noexcept
{
    return pImpl->mPercentile;
}

/// Perturbation
void UncertaintyBound::setPerturbation(
    const std::chrono::microseconds &perturbation) noexcept
{
    pImpl->mPerturbation = perturbation;
}

std::chrono::microseconds UncertaintyBound::getPerturbation() const noexcept
{
    return pImpl->mPerturbation;
}

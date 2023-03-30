#include <chrono>
#include "thresholdDetectorOptions.hpp"

using namespace URTS::Modules::Pickers;

class ThresholdDetectorOptions::ThresholdDetectorOptionsImpl
{
public:
    std::chrono::microseconds mMaximumTriggerDuration{10000000}; // 10 s
    double mOnThreshold{0};
    double mOffThreshold{0};
    int mMinimumGapSize{5};
    bool mHaveOnThreshold{false};
    bool mHaveOffThreshold{false};
};

/// C'tor
ThresholdDetectorOptions::ThresholdDetectorOptions() :
    pImpl(std::make_unique<ThresholdDetectorOptionsImpl> ())
{
}

/// Copy c'tor
ThresholdDetectorOptions::ThresholdDetectorOptions(
    const ThresholdDetectorOptions &options)
{
    *this = options;
}

/// Move c'tor
ThresholdDetectorOptions::ThresholdDetectorOptions(
    ThresholdDetectorOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
ThresholdDetectorOptions&
ThresholdDetectorOptions::operator=(const ThresholdDetectorOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<ThresholdDetectorOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
ThresholdDetectorOptions&
ThresholdDetectorOptions::operator=(ThresholdDetectorOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Destructor
ThresholdDetectorOptions::~ThresholdDetectorOptions() = default;

/// Reset options
void ThresholdDetectorOptions::clear() noexcept
{
    pImpl = std::make_unique<ThresholdDetectorOptionsImpl> ();
}

/// On tolerance
void ThresholdDetectorOptions::setOnThreshold(const double tolerance) noexcept
{
    pImpl->mOnThreshold = tolerance;
    pImpl->mHaveOnThreshold = true;
} 

double ThresholdDetectorOptions::getOnThreshold() const
{
    if (!haveOnThreshold()){throw std::runtime_error("On threshold not set");}
    return pImpl->mOnThreshold;
}

bool ThresholdDetectorOptions::haveOnThreshold() const noexcept
{
    return pImpl->mHaveOnThreshold;
}

/// Off tolerance
void ThresholdDetectorOptions::setOffThreshold(const double tolerance) noexcept
{
    pImpl->mOffThreshold = tolerance;
    pImpl->mHaveOffThreshold = true;
}

double ThresholdDetectorOptions::getOffThreshold() const
{
    if (!haveOffThreshold()){throw std::runtime_error("Off threshold not set");}
    return pImpl->mOffThreshold;
}

bool ThresholdDetectorOptions::haveOffThreshold() const noexcept
{
    return pImpl->mHaveOffThreshold;
}

/// Gap duration
void ThresholdDetectorOptions::setMinimumGapSize(const int duration)
{
    if (duration < 0)
    {
        throw std::invalid_argument("Gap size must be non-zero");
    }
    pImpl->mMinimumGapSize = duration;
}

int ThresholdDetectorOptions::getMinimumGapSize() const noexcept
{
    return pImpl->mMinimumGapSize;
}

/// Max trigger time
void ThresholdDetectorOptions::setMaximumTriggerDuration(
    const std::chrono::microseconds &duration) noexcept
{
    pImpl->mMaximumTriggerDuration = duration;
}

std::chrono::microseconds
ThresholdDetectorOptions::getMaximumTriggerDuration() const noexcept
{
    return pImpl->mMaximumTriggerDuration;
}

/// Parse the initialization file options


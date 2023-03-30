#include <chrono>
#include <limits>
#include "triggerWindow.hpp"

using namespace URTS::Modules::Pickers;

template<class T>
class TriggerWindow<T>::TriggerWindowImpl
{
public:
    std::chrono::microseconds mStartTime{std::numeric_limits<int64_t>::min()};
    std::chrono::microseconds mEndTime{std::numeric_limits<int64_t>::min()};
    std::chrono::microseconds mMaxTime{std::numeric_limits<int64_t>::min()};
    T mStartValue{0};
    T mEndValue{0};
    T mMaximumValue{0};
    bool mHaveStart{false};
    bool mHaveEnd{false};
    bool mHaveMaximum{false};
};

/// C'tor
template<class T>
TriggerWindow<T>::TriggerWindow() :
    pImpl(std::make_unique<TriggerWindowImpl> ())
{
}

/// Copy c'tor
template<class T>
TriggerWindow<T>::TriggerWindow(const TriggerWindow<T> &window)
{
    *this = window;
}

/// Move c'tor
template<class T>
TriggerWindow<T>::TriggerWindow(TriggerWindow<T> &&window) noexcept
{
    *this = std::move(window);
}

/// Copy assignment
template<class T>
TriggerWindow<T>& TriggerWindow<T>::operator=(const TriggerWindow<T> &window)
{
    if (&window == this){return *this;}
    pImpl = std::make_unique<TriggerWindowImpl> (*window.pImpl);
    return *this;
}

/// Move assignment
template<class T>
TriggerWindow<T>& TriggerWindow<T>::operator=(TriggerWindow<T> &&window) noexcept
{
    if (&window == this){return *this;}
    pImpl = std::move(window.pImpl);
    return *this;
}

/// Resets class
template<class T>
void TriggerWindow<T>::clear() noexcept
{
    pImpl = std::make_unique<TriggerWindowImpl> ();
}

/// Destructor
template<class T>
TriggerWindow<T>::~TriggerWindow() = default;

/// Start value
template<class T>
void TriggerWindow<T>::setStart(
    const std::pair<std::chrono::microseconds, T> &start) noexcept
{
    pImpl->mStartTime = start.first;
    pImpl->mStartValue = start.second;
    pImpl->mHaveStart = true;
}

template<class T>
std::pair<std::chrono::microseconds, T> TriggerWindow<T>::getStart() const
{
    if (!haveStart()){throw std::runtime_error("Start not set");}
    return std::pair {pImpl->mStartTime, pImpl->mStartValue};
}

template<class T>
bool TriggerWindow<T>::haveStart() const noexcept
{
    return pImpl->mHaveStart;
}

/// End value
template<class T>
void TriggerWindow<T>::setEnd(
    const std::pair<std::chrono::microseconds, T> &end) noexcept
{
    pImpl->mEndTime = end.first;
    pImpl->mEndValue = end.second;
    pImpl->mHaveEnd = true;
}

template<class T>
std::pair<std::chrono::microseconds, T> TriggerWindow<T>::getEnd() const
{
    if (!haveEnd()){throw std::runtime_error("End not set");}
    return std::pair {pImpl->mEndTime, pImpl->mEndValue};
}

template<class T>
bool TriggerWindow<T>::haveEnd() const noexcept
{
    return pImpl->mHaveEnd;
}

/// Max value
template<class T>
void TriggerWindow<T>::setMaximum(
    const std::pair<std::chrono::microseconds, T> &maximum) noexcept
{
    pImpl->mMaxTime = maximum.first;
    pImpl->mMaximumValue = maximum.second;
    pImpl->mHaveMaximum = true;
}

template<class T>
std::pair<std::chrono::microseconds, T> TriggerWindow<T>::getMaximum() const
{
    if (!haveMaximum()){throw std::runtime_error("Maximum not set");}
    return std::pair {pImpl->mMaxTime, pImpl->mMaximumValue};
}

template<class T>
bool TriggerWindow<T>::haveMaximum() const noexcept
{
    return pImpl->mHaveMaximum;
}


///--------------------------------------------------------------------------///
///                           Template Instantiation                         ///
///--------------------------------------------------------------------------///
template class URTS::Modules::Pickers::TriggerWindow<double>;
template class URTS::Modules::Pickers::TriggerWindow<float>;
template class URTS::Modules::Pickers::TriggerWindow<int>;

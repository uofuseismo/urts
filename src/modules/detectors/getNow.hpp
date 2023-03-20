#ifndef PRIVATE_MODULES_DETECTORS_GET_NOW_HPP
#define PRIVATE_MODULES_DETECTORS_GET_NOW_HPP
#include <chrono>
namespace
{
/// @result Returns the current time.
[[nodiscard]] std::chrono::microseconds getNow()
{
    return std::chrono::duration_cast<std::chrono::microseconds> (
             std::chrono::high_resolution_clock::now().time_since_epoch() );
}
}
#endif

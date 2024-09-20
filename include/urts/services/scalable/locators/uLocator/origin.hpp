#ifndef URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_ORIGIN_HPP
#define URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_ORIGIN_HPP
#include <vector>
#include <chrono>
#include <optional>
#include <memory>
namespace URTS::Services::Scalable::Locators::ULocator
{
 class Arrival;
}
namespace URTS::Services::Scalable::Locators::ULocator
{
/// @class Origin "origin.hpp" "urts/services/scalable/locators/uLocator/origin.hpp"
/// @brief Defines an event origin (latitude, longitude, depth, and time) as
///        well as the defining arrivals.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Origin
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Origin();
    /// @brief Copy constructor.
    /// @param[in] origin  The origin from which to initialize this class.
    Origin(const Origin &origin);
    /// @brief Move constructor.
    /// @param[in,out] origin  The origin from which to initialize this class.
    ///                       On exit, origin's behavior is undefined.
    Origin(Origin &&origin) noexcept;
    /// @}

    /// @name Required Parameters
    /// @{

    /// @brief Sets the event latitude.
    /// @param[in] latitude  The event latitude in degrees. 
    /// @throws std::invalid_argument if the latitude is not in the
    ///         range [-90,90].
    void setLatitude(double latitude);
    /// @result The event latitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates that latitude was set.
    [[nodiscard]] bool haveLatitude() const noexcept;

    /// @brief Sets the event longitude.
    /// @param[in] longitude  The event longitude in degrees.  This is
    ///                       positive east.
    void setLongitude(double longitude) noexcept;
    /// @result The event longitude in degrees.  This will be in the range
    ///         [-180,180).
    /// @throws std::runtime_error if \c haveLongitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates that longitude was set.
    [[nodiscard]] bool haveLongitude() const noexcept;

    /// @brief Sets the event depth in meters.
    /// @param[in] depth  The event depth in meters. 
    /// @note This must be in the range [-8900 - 800000].
    void setDepth(double depth);
    /// @result The event depth in meters.
    [[nodiscard]] double getDepth() const;
    /// @result True indicates the depth was set.
    [[nodiscard]] bool haveDepth() const noexcept;
    /// @brief Toggles the depth as fixed to the free surface.
    /// @param[in] fixedToFreeSurface  If true then the depth was fixed to the
    ///                                free surface.  Otherwise, it is free.
    void toggleDepthFixedToFreeSurface(bool fixedToFreeSurface) noexcept;
    /// @result True indicates the depth was set to the free surface.
    /// @note By default this is false.
    [[nodiscard]] bool depthFixedToFreeSurface() const noexcept;

    /// @brief Sets the event origin time.
    /// @param[in] time  The evnet origin time in seconds (UTC) from the 
    ///                  epoch (Jan 1, 1970).
    void setTime(double time) noexcept;
    /// @brief Sets the event origin time.
    /// @param[in] time  The event origin time microseconds (UTC) from
    ///                  the epoch (Jan 1, 1970).
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @result The event origin time.
    /// @throws std::runtime_error if \c haveTime() is false.
    [[nodiscard]] std::chrono::microseconds getTime() const;
    /// @result True indicates the event time was set.
    [[nodiscard]] bool haveTime() const noexcept;

    /// @brief Sets the arrivals.
    /// @param[in] arrivals  Sets the arrivals from which to build the location.
    ///                      The arrivals must have the network, station,
    ///                      and phase as well as the time.  Additionally,
    ///                      the arrivals must be causal - i.e., no S arrivals
    ///                      can precede P arrivals at a station and no
    ///                      duplicate phases can be added for a station. 
    void setArrivals(const std::vector<Arrival> &arrivals);
    /// @result The arrivals.
    [[nodiscard]] std::vector<Arrival> getArrivals() const noexcept;
    /// @result A reference to the arrivals.
    [[nodiscard]] const std::vector<Arrival> &getArrivalsReference() const noexcept;
    /// @}

    /// @name Optional Parameters
    /// @{

    /// @brief Sets the origin identifier.
    /// @param[in] identifier  The origin identifier.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The origin identifier.
    [[nodiscard]] std::optional<int64_t> getIdentifier() const noexcept;

    /// @brief Sets the weighted root-mean-squared error.
    /// @param[in] wRMSE  The weighted root-mean-squared error in seconds.
    /// @throws std::invalid_argument if wRMSE is negative.
    void setWeightedRootMeanSquaredError(double wRMSE);
    /// @result The weighted root-mean-squared error in seconds.
    [[nodiscard]] std::optional<double> getWeightedRootMeanSquaredError() const;

    /// @brief Sets the largest azimuthal gap in station coverage.
    /// @param[in] gap  The largest azimuthal gap in degrees.
    /// @throws std::invalid_argument if the gap is not in the range (0,360].
    void setAzimuthalGap(double gap);
    /// @result For the given arrivals and epicenter this is the largest 
    ///         azimuthal gap in station coverage in degrees.
    [[nodiscard]] std::optional<double> getAzimuthalGap() const;

    /// @brief Sets the distance from the nearest station to the source.
    /// @param[in] distance  The smallest source-receiver distance in meters.
    /// @throws std::invalid_argument if the distance is negative. 
    void setNearestStationDistance(double distance);
    /// @result The smallest source-station distance in meters.
    /// @throws std::runtime_error if \c haveNearestStationDistance() is false.
    [[nodiscard]] std::optional<double> getNearestStationDistance() const;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor
    ~Origin();
    /// @}

    /// @name Operators and Iterators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] origin  The origin to copy to this.
    /// @result A deep copy of the origin.
    Origin& operator=(const Origin &origin);
    /// @brief Move assignment.
    /// @param[in,out] origin  The origin whose memory will be moved to this.
    ///                        On exit, origin's behavior is undefined.
    /// @result The memory from origin moved to this.
    Origin& operator=(Origin &&origin) noexcept;
    /// @}
private:
    class OriginImpl;
    std::unique_ptr<OriginImpl> pImpl;
};
}
#endif

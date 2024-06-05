#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ORIGIN_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ORIGIN_HPP
#include <memory>
#include <chrono>
#include <vector>
namespace URTS::Services::Scalable::Associators::MAssociate
{
 class Arrival;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class Arrival "arrival.hpp" "urts/services/scalable/associators/massociate/origin.hpp"
/// @brief Defines a seismic event's origin - i.e., the hypocenter, time, and
///        the associated picks.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Messages_MessageFormats
class Origin
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Origin();
    /// @brief Copy constructor.
    /// @param[in] origin  The origin class from which to initialize this class.
    Origin(const Origin &origin);
    /// @brief Move constructor.
    /// @param[in,out] origin  The origin class from which to initialize this
    ///                        class. On exit, origin's behavior is undefined.
    Origin(Origin &&aorigin) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the event's latitude.
    /// @param[in] latitude   The event's latitude in degrees  where latitude
    ///                       increases positive north.
    /// @throws std::invalid_argument if the latitude is not in the
    ///         range [-90,90].
    void setLatitude(double latitude);
    /// @result The event's latitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates that the event's latitude is set.
    [[nodiscard]] bool haveLatitude() const noexcept;

    /// @brief Sets the event's longitude.
    /// @param[in] longitude  The event's longitude in degrees where longitude
    ///                       increases positive east.
    /// @throws std::invalid_argument if the longitude is not in the range
    ///         [-540,540).
    void setLongitude(double longitude);
    /// @result The event's longitude.
    /// @note This will be in the range [0,360).
    /// @throws std::runtime_error if \c haveLongitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates that the longitude is set.
    [[nodiscard]] bool haveLongitude() const noexcept;

    /// @brief The event depth in meters.
    /// @param[in] depth   The event depth.  This increases positive down from
    ///                    some reference such as sea-level.
    /// @note This must be in the range -8600 to 800,000 meters.
    void setDepth(double depth);
    /// @result The event's depth in meters.
    /// @throws std::runtime_error of \c haveDepth() is false.
    [[nodiscard]] double getDepth() const;
    /// @result True indicates that the event depth is set.
    [[nodiscard]] bool haveDepth() const noexcept;

    /// @brief Sets the event's origin time.
    /// @param[in] time  The origin time (UTC) in seconds since the epoch.
    void setTime(double time) noexcept;
    /// @brief Sets the event's origin time.
    /// @param[in] originTime  The origin time (UTC) in microseconds since
    ///                        the epoch. 
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @result The origin time (UTC) in microseconds since the epoch.
    /// @throws std::runtime_error of \c haveTime() is false.
    [[nodiscard]] std::chrono::microseconds getTime() const;
    /// @result True indicates that the origin time is set.
    [[nodiscard]] bool haveTime() const noexcept;
    /// @}

    /// @param[in] arrivals  The arrivals defining this origin.
    void setArrivals(const std::vector<Arrival> &arrivals);
    /// @result The arrivals defining this origin.
    [[nodiscard]] std::vector<Arrival> getArrivals() const noexcept;
    /// @result A reference to the arrivals.
    [[nodiscard]] const std::vector<Arrival> &getArrivalsReference() const noexcept;

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases all memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Origin();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] origin  The origin class to copy to this.
    /// @result A deep copy of the origin.
    Origin& operator=(const Origin &origin);
    /// @brief Move assignment.
    /// @param[in,out] origin  The origin class whose memory will be moved to
    ///                        this.  On exit origin's behavior is undefined.
    /// @result The memory from origin moved to this.
    Origin& operator=(Origin &&origin) noexcept;
    /// @}
private:
    class OriginImpl;
    std::unique_ptr<OriginImpl> pImpl;
};
}
#endif

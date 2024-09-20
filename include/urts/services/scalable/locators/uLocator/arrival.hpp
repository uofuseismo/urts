#ifndef URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_ARRIVAL_HPP
#define URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_ARRIVAL_HPP
#include <memory>
#include <chrono>
#include <vector>
#include <optional>
#include <string>
namespace URTS::Broadcasts::Internal::Pick
{
 class UncertaintyBound;
}
namespace URTS::Services::Scalable::Locators::ULocator
{
/// @class Arrival "arrival.hpp" "urts/services/scalable/locators/uLocate/arrival.hpp"
/// @brief Defines an arrival from which to build the location.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Arrival
{
public:
    /// @brief Define's the seismic phase of this arrival.
    enum class Phase : int8_t
    {
        P = 0,  /*!< The phase arrival is for a P phase. */
        S = 1   /*!< The phase arrival is for an S phase. */ 
    }; 
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Arrival();
    /// @brief Copy constructor.
    /// @param[in] arrival  The arrival class from which to initialize
    ///                     this class.
    Arrival(const Arrival &arrival);
    /// @brief Move constructor.
    /// @param[in,out] arrival  The arrival class from which to initialize this
    ///                      class.  On exit, arrival's behavior is undefined.
    Arrival(Arrival &&arrival) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] arrival  The arrival class to copy to this.
    /// @result A deep copy of the arrival.
    Arrival& operator=(const Arrival &arrival);
    /// @brief Move assignment.
    /// @param[in,out] arrival  The arrival class whose memory will be moved to
    ///                      this.  On exit arrival's behavior is undefined.
    /// @result The memory from arrival moved to this.
    Arrival& operator=(Arrival &&arrival) noexcept;
    /// @}

    /// @name Required Information
    /// @{

    /// @brief Sets the phase onset time.
    /// @param[in] time  The time (UTC) of thearrival.  This is measured in
    ///                  seconds since the epoch (Jan 1, 1970).
    /// @note This time will be rounded to the nearest microsecond.
    void setTime(double time) noexcept;
    /// @brief Sets the phase onset time.
    /// @param[in] time  The time (UTC) of the arrival.  This is measured in
    ///                  microseconds since the epoch (Jan 1, 1970).
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @result The time (UTC) of the arrival in microseconds since the epoch.
    /// @throws std::runtime_error if \c haveTime() is false.
    [[nodiscard]] std::chrono::microseconds getTime() const;
    /// @result True indicates that the arrival time was set.
    [[nodiscard]] bool haveTime() const noexcept;
 
    /// @brief Sets the network code on which the pick was made.
    /// @param[in] network  The network code.
    /// @throws std::invalid_argument if network is empty.
    void setNetwork(const std::string &network);
    /// @result The network code.
    /// @throws std::runtime_error if \c haveNetwork() is false.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates that the network was set.
    [[nodiscard]] bool haveNetwork() const noexcept;

    /// @brief Sets the station name on which the pick was made.
    /// @param[in] station   The station name.
    /// @throws std::invalid_argument if station is empty.
    void setStation(const std::string &station); 
    /// @result The station name.
    /// @throws std::runtime_error if \c haveStation() is false.
    [[nodiscard]] std::string getStation() const;
    /// @result True indicates that the station name was set.
    [[nodiscard]] bool haveStation() const noexcept;

    /// @brief Sets the arrival's seismic phase.
    /// @param[in] phase  The phase - e.g., P or S.
    void setPhase(const Phase phase) noexcept;
    /// @result The phase.
    /// @throws std::runtime_error if \c havePhase() is false.
    [[nodiscard]] Phase getPhase() const; 
    /// @result True indicates the phase was set.
    [[nodiscard]] bool havePhase() const noexcept;
    /// @}

    /// @name Optional Information
    /// @{

    /// @brief Sets the travel time in seconds.
    /// @param[in] travelTime  The theoretical travel time.
    /// @throws std::invalid_argument if the travel time is negative.
    void setTravelTime(double travelTime);
    /// @result The travel time in seconds.
    [[nodiscard]] std::optional<double> getTravelTime() const noexcept;

    /// @brief Sets a unique arrival identification number.
    /// @param[in] identifier   The unique arrival identification number.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The unique arrival identification number.
    [[nodiscard]] std::optional<int64_t> getIdentifier() const noexcept;

    /// @brief Sets the standard error of the arrival time.
    /// @param[in] standardError  The standard error of the arrival time
    ///                           in seconds.
    /// @throws std::invalid_argument if the standard error is not positive.
    void setStandardError(double standardError);
    /// @result The standard error of the arrival time in seconds.
    [[nodiscard]] std::optional<double> getStandardError() const noexcept;
    /// @brief Sets the lower and upper uncertainty bound on the arrival time.
    /// @throws std::invalid_argument if the lowerBound exceeds the upperBound.
    /// @note This more general representation of uncertainty will be converted
    ///       to a standard error under a Gaussian assumption.
    void setLowerAndUpperUncertaintyBound(const std::pair<URTS::Broadcasts::Internal::Pick::UncertaintyBound, URTS::Broadcasts::Internal::Pick::UncertaintyBound> &lowerAndUpperBound);
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases all memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Arrival();
    /// @}
private:
    class ArrivalImpl;
    std::unique_ptr<ArrivalImpl> pImpl;
};
}
#endif

#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ARRIVAL_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ARRIVAL_HPP
#include <memory>
#include <chrono>
#include <optional>
#include <string>
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class Arrival "arrival.hpp" "urts/services/scalable/associators/massociate/arrival.hpp"
/// @brief Defines an associated pick.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Messages_MessageFormats
class Arrival
{
public:
    /// @brief Defines the phase identified by the associator.
    enum class Phase
    {   
        P = 0,  /*!< This is likely a P phase arrival. */
        S = 1   /*!< This is likely an S phase arrival. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Arrival();
    /// @brief Copy constructor.
    /// @param[in] arrival  The arrival class from which to initialize this
    ///                     class.
    Arrival(const Arrival &arrival);
    /// @brief Move constructor.
    /// @param[in,out] arrival  The arrival class from which to initialize this
    ///                         class. On exit, arrival's behavior is undefined.
    Arrival(Arrival &&arrival) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

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

    /// @brief Sets the channel name on which the pick was made.
    /// @param[in] channel  The channel name.
    /// @throws std::invalid_argument if channel is empty.
    void setChannel(const std::string &channel);
    /// @result The channel name.
    /// @throws std::runtime_error if the channel was not set.
    [[nodiscard]] std::string getChannel() const;
    /// @result True indicates that the channel was set.
    [[nodiscard]] bool haveChannel() const noexcept;

    /// @brief Sets the location code.
    /// @param[in] location  The location code.
    /// @throws std::invalid_argument if location is empty.
    void setLocationCode(const std::string &location);
    /// @brief Sets the channel code on which the pick was made.
    /// @throws std::runtime_error if \c haveLocationCode() is false.
    [[nodiscard]] std::string getLocationCode() const;
    /// @result True indicates that the location code was set.
    [[nodiscard]] bool haveLocationCode() const noexcept;

    /// @brief Sets the pick time.
    /// @param[in] time  The time (UTC) of the pick.  This is measured in
    ///                  seconds since the epoch (Jan 1, 1970).
    /// @note This time will be rounded to the nearest microsecond.
    void setTime(double time) noexcept;
    /// @brief Sets the pick time.
    /// @param[in] time  The time (UTC) of the pick.  This is measured in
    ///                  microseconds since the epoch (Jan 1, 1970).
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @result The time (UTC) of the pick in microseconds since the epoch.
    /// @throws std::runtime_error if \c haveTime() is false.
    [[nodiscard]] std::chrono::microseconds getTime() const;
    /// @result True indicates that the pick time was set.
    [[nodiscard]] bool haveTime() const noexcept;

    /// @brief Sets the arrival's seismic phase.
    /// @param[in] phase  The phase - e.g., P or S.
    void setPhase(Phase phase) noexcept;
    /// @result The phase of the arrival.
    /// @throws std::runtime_error if \c havePhase() is false.
    [[nodiscard]] Phase getPhase() const;
    /// @result True indicates the phase was set.
    [[nodiscard]] bool havePhase() const noexcept;
    /// @} 

    /// @name Optional Information
    /// @{

    /// @brief Sets the arrival identification number.
    /// @param[in] identifier   The arrival identification number.  This will
    ///                         match the identifier on the input pick.
    void setIdentifier(uint64_t identifier) noexcept;
    /// @result The arrival identification number.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] uint64_t getIdentifier() const;
    /// @result True indicates that the arrival identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;

    /// @brief Sets the travel time from the source to the receiver.
    /// @param[in] travelTime   The source-to-receiver travel time in seconds.
    /// @throws std::invalid_argument if the travel time is not positive.
    void setTravelTime(double travelTime);
    /// @result Gets the travel time from the source to the receiver in seconds.
    ///         Returns nullptr if the travel time was not set.
    [[nodiscard]] std::optional<double> getTravelTime() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases all memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Arrival();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] arrival  The arrival class to copy to this.
    /// @result A deep copy of the arrival.
    Arrival& operator=(const Arrival &arrival);
    /// @brief Move assignment.
    /// @param[in,out] arrival  The arrival class whose memory will be moved to
    ///                         this.  On exit arrival's behavior is undefined.
    /// @result The memory from arrival moved to this.
    Arrival& operator=(Arrival &&arrival) noexcept;
    /// @}
private:
    class ArrivalImpl;
    std::unique_ptr<ArrivalImpl> pImpl;
};
}
#endif

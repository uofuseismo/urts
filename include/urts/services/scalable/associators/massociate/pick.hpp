#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_PICK_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_PICK_HPP
#include <memory>
#include <chrono>
#include <string>
namespace URTS::Broadcasts::Internal::Pick
{
 class Pick;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class Pick "pick.hpp" "urts/broadcasts/internal/pick/pick.hpp"
/// @brief Defines a pick (an unassociated arrival).
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Messages_MessageFormats
class Pick
{
public:
    /// @brief Defines the phase hint for the associator.
    enum class PhaseHint
    {
        P = 0,  /*!< This is likely a P phase arrival. */
        S = 1   /*!< This is likely an S phase arrival. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Pick();
    /// @brief Copy constructor.
    /// @param[in] pick  The pick class from which to initialize this class.
    Pick(const Pick &pick);
    /// @brief Creates a pick from a pick message.
    /// @param[in] pick  The pick message from which to construct this class.
    /// @throws std::invalid_argument if a requisite piece of information is
    ///         not set.
    explicit Pick(const URTS::Broadcasts::Internal::Pick::Pick &pick);
    /// @brief Move constructor.
    /// @param[in,out] pick  The pick class from which to initialize this
    ///                      class.  On exit, pick's behavior is undefined.
    Pick(Pick &&pick) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] pick  The pick class to copy to this.
    /// @result A deep copy of the pick.
    Pick& operator=(const Pick &pick);
    /// @brief Move assignment.
    /// @param[in,out] pick  The pick class whose memory will be moved to
    ///                      this.  On exit pick's behavior is undefined.
    /// @result The memory from pick moved to this.
    Pick& operator=(Pick &&pick) noexcept;
    /// @}

    /// @name Required Information
    /// @{

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
    /// @}
 
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

    /// @brief Sets a guess of the arrival's seismic phase.
    /// @param[in] phase  The phase hint - e.g., P or S.
    void setPhaseHint(PhaseHint phaseHint) noexcept;
    /// @result The phase hint.
    /// @throws std::runtime_error if \c havePhaseHint() is false.
    [[nodiscard]] PhaseHint getPhaseHint() const;
    /// @result True indicates the phase hint was set.
    [[nodiscard]] bool havePhaseHint() const noexcept;
    /// @} 

    /// @name Optional Information
    /// @{

    /// @brief Sets a unique pick identification number.
    /// @param[in] identifier   The unique pick identification number.
    void setIdentifier(uint64_t identifier) noexcept;
    /// @result The unique pick identification number.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] uint64_t getIdentifier() const;
    /// @result True indicates that the pick identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;

    /// @brief Sets the standard error for the pick.
    /// @param[in] error  The standard error in seconds.
    /// @throws std::invalid_argument if the standard error is not positive.
    void setStandardError(const double error);
    /// @result Gets the standard error for the pick.
    [[nodiscard]] double getStandardError() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases all memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Pick();
    /// @}
private:
    class PickImpl;
    std::unique_ptr<PickImpl> pImpl;
};
}
#endif

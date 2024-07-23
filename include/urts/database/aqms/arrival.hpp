#ifndef URTS_DATABASE_AQMS_ARRIVAL_HPP
#define URTS_DATABASE_AQMS_ARRIVAL_HPP
#include <memory>
#include <string>
#include <chrono>
#include <optional>
namespace URTS::Database::AQMS
{
/// @class Arrival "arrival.hpp" "urts/database/aqms/arrival.hpp"
/// @brief Defines an arrival (a pick that is associated to an origin) in AQMS.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class Arrival
{
public:
    /// @brief The arrival's review status.
    enum class ReviewFlag
    {
        Automatic = 0,  /*!< This is an automatically generated arrival. */
        Human = 1,      /*!< This is a human generated arrival. */
        Finalized = 2   /*!< This is a finalized arrival.  This does not appear
                             to be used. */
    }; 
    /// @brief The first motion of the arrival.
    enum class FirstMotion
    {
        Up = 1, /*!< The first motion is up (compression). */
        Down =-1, /*!< The first motion is down (dilitation). */
        Unknown = 0  /*!< The first motion is unknown. */
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
    /// @param[in,out] arrival  The arrival class from which to initialize 
    ///                         this class.  On exit, arrival's behavior is
    ///                         undefined.
    Arrival(Arrival &&arrival) noexcept;
    /// @}

    /// @name Properties Required by Database
    /// @{

    /// @brief Sets the authority that created the arrival.
    /// @param[in] authority   The authority that created the arrival.
    void setAuthority(const std::string &authority);
    /// @result The authority that created the arrival.
    /// @throws std::runtime_error if \c haveAuthority() is false.
    [[nodiscard]] std::string getAuthority() const;
    /// @result True indicates the authority was set.
    [[nodiscard]] bool haveAuthority() const noexcept;

    /// @brief Sets the station name.
    /// @param[in] station  The station name.
    void setStation(const std::string &station);
    /// @result The station name.
    /// @throws std::invalid_argument if \c haveStation() is false.
    [[nodiscard]] std::string getStation() const; 
    /// @result True indicates that the station name was set.
    [[nodiscard]] bool haveStation() const noexcept;

    /// @brief Sets the arrival time.
    /// @param[in] time  The arrival time (UTC) in microseconds since the epoch.
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @param[in] time   The arrival time (UTC) in seconds since the epoch.
    void setTime(double time) noexcept;
    /// @result The arrival time (UTC) in seconds since the epoch.
    /// @throws std::runtime_error if \c haveTime() is false.
    [[nodiscard]] double getTime() const;
    /// @result True indicates the arrival time was set.
    [[nodiscard]] bool haveTime() const noexcept;
    /// @}

    /// @name Other Properties
    /// @{

    /// @brief Sets the network name.
    /// @param[in] network  The network name.
    void setNetwork(const std::string &network);
    /// @result The network name.
    [[nodiscard]] std::optional<std::string> getNetwork() const noexcept;

    /// @brief Sets the SEED channel name.
    /// @param[in] channel  The channel name.
    void setSEEDChannel(const std::string &channel);
    /// @result The SEED channel name.
    [[nodiscard]] std::optional<std::string> getSEEDChannel() const noexcept;

    /// @brief Sets the location code.
    /// @param[in] locationCode  The location code.
    void setLocationCode(const std::string &locationCode);
    /// @result The location code.
    [[nodiscard]] std::optional<std::string> getLocationCode() const noexcept;

    /// @brief Sets the arrival identifier. 
    /// @param[in] identifier  The arrival identifier.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The arrival identifier.
    [[nodiscard]] std::optional<int64_t> getIdentifier() const noexcept;

    /// @brief Sets the phase arrival.
    /// @param[in] phase   The phase - e.g., P or S.
    void setPhase(const std::string &phase);
    /// @result The phase.
    [[nodiscard]] std::optional<std::string> getPhase() const noexcept;

    /// @brief Sets the quality of the arrival.  One is the highest quality
    ///        and zero is the lowest quality.
    /// @param[in] quality  The quality of the arrival.
    void setQuality(double quality);
    /// @result The arrival's quality.
    [[nodiscard]] std::optional<double> getQuality() const noexcept;

    /// @brief Sets a secondary identifier to indicate where the arrival
    ///        was made.
    /// @param[in] subsource   The subsource - e.g., Jiggle or RT1.
    void setSubSource(const std::string &subsource);
    /// @result The secondary identifier indicating where the arrival was made.
    [[nodiscard]] std::optional<std::string> getSubSource() const noexcept;
    
    /// @brief Sets the arrival's review flag.
    /// @param[in] reviewFlag  The arrival's review status.
    void setReviewFlag(ReviewFlag reviewFlag) noexcept;
    /// @result The arrival's review status.
    [[nodiscard]] std::optional<ReviewFlag> getReviewFlag() const noexcept;

    /// @brief Sets the arrival's first motion.
    /// @param[in] firstMotion   The first motion of the arrival.
    void setFirstMotion(FirstMotion firstMotion) noexcept;
    /// @result The first motion of the arrival.   By default this is unknown.
    [[nodiscard]] FirstMotion getFirstMotion() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Arrival();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] arrival  The arrival to copy to this.
    /// @result A deep copy of the arrival.
    Arrival& operator=(const Arrival &arrival);
    /// @brief Move assignment.
    /// @param[in,out] arrival  The arrival whose memory will be moved to this.
    ///                         On exit, arrival's behavior is undefined.
    /// @result The memory from arrival moved to this.
    Arrival& operator=(Arrival &&arrival) noexcept;
    /// @}
private:
    class ArrivalImpl;
    std::unique_ptr<ArrivalImpl> pImpl;
};
[[nodiscard]] std::string toInsertString(const Arrival &arrival);
}
#endif 

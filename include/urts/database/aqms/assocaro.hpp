#ifndef URTS_DATABASE_AQMS_ASSOCARO_HPP
#define URTS_DATABASE_AQMS_ASSOCARO_HPP
#include <string>
#include <memory>
#include <optional>
namespace URTS::Database::AQMS
{
/// @class AssocArO "assocaro.hpp" "urts/database/aqms/assocaro.hpp"
/// @brief Associates an arrival to an origin.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class AssocArO
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
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    AssocArO();
    /// @brief Copy constructor.
    /// @param[in] assocaro  The origin/arrival association class from which to
    ///                      initialize this class.
    AssocArO(const AssocArO &assocaro);
    /// @brief Move constructor.
    /// @param[in,out] assocaro  The origin/arrival association class from which
    ///                          to initialize this class.  On exit, assocaro's
    ///                          behavior is undefined.
    AssocArO(AssocArO &&assocaro) noexcept;
    /// @}

    /// @name Parameters Required by Database
    /// @{

    /// @brief Sets the authority that created the origin/arrival.
    /// @param[in] authority   The authority that created the origin/arrival.
    void setAuthority(const std::string &authority);
    /// @result The authority that created the origin/arrival.
    /// @throws std::runtime_error if \c haveAuthority() is false.
    [[nodiscard]] std::string getAuthority() const;
    /// @result True indicates the authority was set.
    [[nodiscard]] bool haveAuthority() const noexcept;

    /// @brief Sets the origin identifier.
    /// @param[in] identifier  The origin identifier.
    void setOriginIdentifier(int64_t identifier) noexcept;
    /// @result The origin identifier.
    /// @throws std::runtime_error if \c haveOriginIdentifier() is false.
    [[nodiscard]] int64_t getOriginIdentifier() const;
    /// @result True indicates the origin identifier was set.
    [[nodiscard]] bool haveOriginIdentifier() const noexcept;

    /// @brief Sets the arrival identifier.
    /// @param[in] identifier  The arrival identifier.
    void setArrivalIdentifier(int64_t identifier) noexcept;
    /// @result The arrival identifier.
    /// @throws std::runtime_error if \c haveArrivalIdentifier() is false.
    [[nodiscard]] int64_t getArrivalIdentifier() const;
    /// @result True indicates the arrival identifier was set.
    [[nodiscard]] bool haveArrivalIdentifier() const noexcept;
    /// @}

    /// @name Other Properties
    /// @{

    /// @brief Sets a secondary identifier to indicate where the origin
    ///        was originally created.
    /// @param[in] subsource   The subsource - e.g., Jiggle or RT1.
    void setSubSource(const std::string &subsource);
    /// @result The secondary identifier indicating where the origin was 
    ///         originally created.
    [[nodiscard]] std::optional<std::string> getSubSource() const noexcept;

    /// @brief Sets the phase arrival.
    /// @param[in] phase   The phase - e.g., P or S.
    void setPhase(const std::string &phase);
    /// @result The phase.
    [[nodiscard]] std::optional<std::string> getPhase() const noexcept;

    /// @brief Sets the source-to-receiver distance.
    /// @param[in] distance  The source-to-receiver distance in kilometers.
    void setSourceReceiverDistance(double distance);
    /// @result The source-to-receiver distance in kilometers.
    [[nodiscard]] std::optional<double> getSourceReceiverDistance() const noexcept;

    /// @brief Sets the source-to-station azimuth.
    /// @param[in] azimuth  The source-to-station azimuth measured positive 
    ///                     east of north in degrees.
    void setSourceToReceiverAzimuth(double azimuth);
    /// @result The source-to-receiver azimuth in degrees.
    [[nodiscard]] std::optional<double> getSourceToReceiverAzimuth() const noexcept;

    /// @brief The input weight.
    /// @param[in] weight   The input weight used in hypoinverse.
    ///                     1 is the highest weight and 0 is not used. 
    void setInputWeight(double weight);
    /// @result The input weight in the location algorithm.
    [[nodiscard]] std::optional<double> getInputWeight() const noexcept;

    /// @brief Sets the travel time residual.
    /// @param[in] residual   The travel time residual in seconds.
    void setTravelTimeResidual(double residual) noexcept;
    /// @result The travel time residual in seconds.
    [[nodiscard]] std::optional<double> getTravelTimeResidual() const noexcept;

    /// @brief Sets the takeoff angle.
    /// @param[in] angle   The takeoff angle.  Here 0 is nadir and 180 is up
    ///                     towards surface.
    void setTakeOffAngle(double angle);
    /// @result The take off angle in degrees.
    [[nodiscard]] std::optional<double> getTakeOffAngle() const noexcept;

    /// @brief Sets the arrival's review flag.
    /// @param[in] reviewFlag  The arrival's review status.
    void setReviewFlag(ReviewFlag reviewFlag) noexcept;
    /// @result The arrival's review status.
    [[nodiscard]] std::optional<ReviewFlag> getReviewFlag() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~AssocArO();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] assocaro  The origin/arrival assocation class to copy to
    ///                      this.
    /// @result A deep copy of assocaro.
    AssocArO& operator=(const AssocArO &assocaro);
    /// @brief Move assignment.
    /// @param[in,out] assocaro  The origin/arrival assocation class whose
    ///                          memory will be moved to this.  On exit,
    ///                          assocaro's behavior is undefined.
    /// @result The memory from assocaro moved to this.
    AssocArO& operator=(AssocArO &&assocaro) noexcept;
    /// @}
private:
    class AssocArOImpl;
    std::unique_ptr<AssocArOImpl> pImpl;
};
[[nodiscard]] std::string toInsertString(const AssocArO &assocaro);
}
#endif

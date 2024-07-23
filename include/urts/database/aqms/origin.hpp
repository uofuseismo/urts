#ifndef URTS_DATABASE_AQMS_ORIGIN_HPP
#define URTS_DATABASE_AQMS_ORIGIN_HPP
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <optional>
namespace URTS::Database::AQMS
{
/// @class Origin "origin.hpp" "urts/database/aqms/origin.hpp"
/// @brief Defines an origin in AQMS.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class Origin
{
public:
    /// @brief The origin's review status.
    enum class ReviewFlag
    {   
        Automatic = 0,  /*!< This is an automatically generated origin. */
        Incomplete = 1, /*!< This has been reviewed by a human but is in an incomplete state. */
        Human = 2,      /*!< This is a human reviewed origin. */
        Finalized = 3,  /*!< This is a finalized origin. */
        Cancelled = 4   /*!< This is a cancelled origin. */
    };  
    /// @brief The geographic type of the origin.
    enum class GeographicType
    {   
        Local = 0,       /*!< This a local event. */
        Regional = 1,    /*!< This is a regional event. */
        Teleseismic = 2, /*!< This is a teleseismic event. */
    };  
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Origin();
    /// @brief Copy constructor.
    /// @param[in] origin  The origin class from which to initialize this
    ///                    class.
    Origin(const Origin &origin);
    /// @brief Move constructor.
    /// @param[in,out] origin  The origin class from which to initialize 
    ///                        this class.  On exit, origin's behavior is
    ///                        undefined.
    Origin(Origin &&origin) noexcept;
    /// @}

    /// @name Properties Required by the Database
    /// @{

    /// @brief Sets the authority that created the origin.
    /// @param[in] authority   The authority that created the origin.
    void setAuthority(const std::string &authority);
    /// @result The authority that created the origin.
    /// @throws std::runtime_error if \c haveAuthority() is false.
    [[nodiscard]] std::string getAuthority() const;
    /// @result True indicates the authority was set.
    [[nodiscard]] bool haveAuthority() const noexcept;

    /// @brief Sets the origin identifier.
    /// @param[in] identifier  The origin identifier.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The origin identifier.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] int64_t getIdentifier() const;
    /// @result True indicates the origin identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;

    /// @brief Sets the event identifier.
    /// @param[in] identifier  The event identifier.
    void setEventIdentifier(int64_t identifier) noexcept;
    /// @result The event identifier.
    /// @throws std::runtime_error if \c haveEventIdentifier() is false.
    [[nodiscard]] int64_t getEventIdentifier() const;
    /// @result True indicates the event identifier was set.
    [[nodiscard]] bool haveEventIdentifier() const noexcept;

    /// @brief Sets the origin time.
    /// @param[in] time  The origin time (UTC) in microseconds since the epoch.
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @param[in] time   The origin time (UTC) in seconds since the epoch.
    void setTime(double time) noexcept;
    /// @result The origin time (UTC) in seconds since the epoch.
    /// @throws std::runtime_error if \c haveTime() is false.
    [[nodiscard]] double getTime() const;
    /// @result True indicates the origin time was set.
    [[nodiscard]] bool haveTime() const noexcept;

    /// @brief Sets the origin's latitude.
    /// @param[in] latitude  The origin's latitude in degrees.
    /// @throws std::invalid_argument if the latitude is not in
    ///         the range [-90,90].
    void setLatitude(double latitude);
    /// @result The origin's latitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates that latitude was set.
    [[nodiscard]] bool haveLatitude() const noexcept;

    /// @brief Sets the origin's longitude.
    /// @param[in] longitude  The origin's latitude in degrees.
    void setLongitude(double longitude) noexcept;
    /// @result The origin's longitude in degrees.
    /// @throws std::runtime_error if \c haveLongitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates that longitude was set.
    [[nodiscard]] bool haveLongitude() const noexcept;

    /// @brief Sets the origin as bogus.
    void setBogus() noexcept;
    /// @brief Sets the origin as not bogus.
    void unsetBogus() noexcept;
    /// @result True indicates the origin is bogus.  By default this is false.
    [[nodiscard]] bool isBogus() const noexcept;
    /// @}

    /// @name Other Properties
    /// @{

    /// @brief Sets the preferred magnitude identifier.
    /// @param[in] identifier  The identifier of the magnitude preferred by this
    ///                        origin.
    void setPreferredMagnitudeIdentifier(int64_t identifier) noexcept;
    /// @result The identifier of the preferred magnitude.
    [[nodiscard]] std::optional<int64_t> getPreferredMagnitudeIdentifier() const noexcept; 

    /// @brief Sets the preferred mechanism identifier.
    /// @param[in] identifier  The identifier of the mechanism preferred by this
    ///                        origin.
    void setPreferredMechanismIdentifier(int64_t identifier) noexcept;
    /// @result The identifier of the preferred mechanism.
    [[nodiscard]] std::optional<int64_t> getPreferredMechanismIdentifier() const noexcept; 

    /// @brief Sets the origin's depth.
    /// @param[in] depth   The origin's depth in kilometers below the geoid.
    void setDepth(double depth);
    /// @result The origin's depth in kilometers.
    [[nodiscard]] std::optional<double> getDepth() const noexcept;

    /// @brief Sets the geograhpic type.
    /// @param[in] geographicType   The geographic type of the event.
    void setGeographicType(GeographicType geographicType) noexcept;
    /// @result The geographic type.
    [[nodiscard]] std::optional<GeographicType> getGeographicType() const noexcept;

    /// @brief Sets the location algorithm used to compute the origin.
    /// @param[in] algorithm   The location algorithm.
    void setAlgorithm(const std::string &algorithm);
    /// @result The location algorithm.
    [[nodiscard]] std::optional<std::string> getAlgorithm() const noexcept;

    /// @brief Sets a secondary identifier to indicate where the origin
    ///        was originally created.
    /// @param[in] subsource   The subsource - e.g., Jiggle or RT1.
    void setSubSource(const std::string &subsource);
    /// @result The secondary identifier indicating where the origin was 
    ///         originally created.
    [[nodiscard]] std::optional<std::string> getSubSource() const noexcept;

    /// @brief Sets the maximum station azimuthal gap.
    /// @param[in] gap   The azimuthal gap in degrees.
    void setGap(double gap);
    /// @result The maximum station azimuthal gap in degrees.
    [[nodiscard]] std::optional<double> getGap() const noexcept;

    /// @brief Sets the epicentral distance to the nearest station to the
    ///        origin.
    /// @param[in] distance  The distance to nearest station in kilometers.
    void setDistanceToNearestStation(double distance);
    /// @result The epicentral distance to the nearest station in kilometers.
    [[nodiscard]] std::optional<double> getDistanceToNearestStation() const noexcept;

    /// @brief Sets the weighted root mean squared error of the solution.
    /// @param[in] wrmse  The weighted root mean squared error in seconds.
    void setWeightedRootMeanSquaredError(double wrmse);
    /// @result The weighted root mean squared error of the solution.
    [[nodiscard]] std::optional<double> getWeightedRootMeanSquaredError() const noexcept;

    /// @brief Sets the origin's review flag.
    /// @param[in] reviewFlag  The origin's review status.
    void setReviewFlag(ReviewFlag reviewFlag) noexcept;
    /// @result The origin's review status.
    [[nodiscard]] std::optional<ReviewFlag> getReviewFlag() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Origin();
    /// @}

    /// @name Operators
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
[[nodiscard]] std::string toInsertString(const Origin &origin);
}
#endif

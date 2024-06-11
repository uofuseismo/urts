#ifndef URTS_BROADCASTS_INTERNAL_ORIGIN_ORIGIN_HPP
#define URTS_BROADCASTS_INTERNAL_ORIGIN_ORIGIN_HPP
#include <vector>
#include <chrono>
#include <memory>
#include <umps/messageFormats/message.hpp>
namespace URTS::Broadcasts::Internal::Origin
{
 class Arrival;
}
namespace URTS::Broadcasts::Internal::Origin
{
/// @brief Defines an origin which is an event location built from arrivals.
class Origin : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Sets the review status as either being purely automatic - i.e.,
    ///        there was no review, or the pick was manually reviewed, 
    ///        possibly edited, then published.
    enum ReviewStatus : int8_t
    {   
        Automatic = 0, /*!< This is an automatically generated origin. */
        Manual    = 1  /*!< This is origin was created or reviewed by a human. */
    };  
private:
    using VectorType = std::vector<Arrival>;
public:
    using iterator = typename VectorType::iterator;
    using const_iterator = typename VectorType::const_iterator;
public:
    /// @name Constructors
    /// @{

    /// @brief Construtor.
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

    /// @brief Sets the event origin time.
    /// @param[in] time  The event origin time microseconds (UTC) from
    ///                  the epoch (Jan 1, 1970).
    void setTime(const std::chrono::microseconds &time) noexcept;
    /// @result The event origin time.
    /// @throws std::runtime_error if \c haveTime() is false.
    [[nodiscard]] std::chrono::microseconds getTime() const;
    /// @result True indicates the event time was set.
    [[nodiscard]] bool haveTime() const noexcept;

    /// @brief Origin identifier.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The origin identifier.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] int64_t getIdentifier() const;
    /// @result True indicates the origin identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;
    /// @}

    /// @name Optional Parameters
    /// @{

    /// @brief Sets the algorithms that created the origin.
    /// @param[in] algorithms   The origin creation algorithms.
    void setAlgorithms(const std::vector<std::string> &algorithms) noexcept;
    /// @result The algorithms that created the origin.
    [[nodiscard]] std::vector<std::string> getAlgorithms() const noexcept;

    /// @brief Sets the arrivals corresponding to this origin.
    /// @param[in,out] arrivals  The arrivals.  On exit, the behavior of
    ///                          arrivals is undefined.
    void setArrivals(std::vector<Arrival> &&arrivals);
    /// @brief Sets the arrivals corresponding to this origin.
    /// @param[in] arrivals  The arrivals.
    void setArrivals(const std::vector<Arrival> &arrivals);
    //// @result The arrivals making up the origin.
    [[nodiscard]] std::vector<Arrival> getArrivals() const noexcept;
    /// @result The arrivals making up the origin.
    [[nodiscard]] const std::vector<Arrival> &getArrivalsReference() const noexcept;

    /// @brief Since interactive algorithms allow for human revision we denote
    ///        a status flag as either human reviewed or automatic.
    /// @param[in] status  The review status of the pick.
    void setReviewStatus(ReviewStatus status) noexcept;
    /// @result The review status.  By default this is automatic.
    [[nodiscard]] ReviewStatus getReviewStatus() const noexcept;
    /// @}

    /// @name Message Abstract Base Class Properties
    /// @{

    /// @brief Converts the pick class to a string message.
    /// @result The class expressed as a string message.
    /// @throws std::runtime_error if the required information is not set. 
    /// @note Though the container is a string the message need not be
    ///       human readable.
    [[nodiscard]] std::string toMessage() const final;
    /// @brief Creates the class from a message.
    /// @param[in] message  The contents of the message.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if data.empty() is true.
    void fromMessage(const std::string &message) final;
    /// @brief Creates the class from a message.
    /// @param[in] data    The contents of the message.  This is an
    ///                    array whose dimension is [length] 
    /// @param[in] length  The length of data.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if data is NULL or length is 0. 
    void fromMessage(const char *data, size_t length) final;
    /// @result A message type indicating this is a pick message.
    [[nodiscard]] std::string getMessageType() const noexcept final;
    /// @result A copy of this class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> clone() const final;
    /// @result An uninitialized instance of this class. 
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> createInstance() const noexcept final;
    /// @result The message version.
    [[nodiscard]] std::string getMessageVersion() const noexcept final;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor
    ~Origin() override;
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

    /// @result A reference to the first arrival.
    iterator begin();
    /// @result A constant reference to the first arrival.
    const_iterator begin() const noexcept;
    /// @result A constant reference to the first arrival.
    const_iterator cbegin() const noexcept;

    /// @result A reference to the last arrival. 
    iterator end();
    /// @result A reference to the last arrival.
    const_iterator end() const noexcept;
    /// @result A const reference to the arrival.
    const_iterator cend() const noexcept;
    /// @}
private:
    class OriginImpl;
    std::unique_ptr<OriginImpl> pImpl;
};
}
#endif

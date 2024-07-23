#ifndef URTS_DATABASE_AQMS_EVENT_HPP
#define URTS_DATABASE_AQMS_EVENT_HPP
#include <vector>
#include <memory>
#include <string>
#include <optional>
namespace URTS::Database::AQMS
{
/// @class Event "event.hpp" "urts/database/aqms/event.hpp"
/// @brief Defines an event in AQMS.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class Event
{
public:
    /// @brief The event's type.
    enum class Type
    {   
        Unknown,           /*!< Unknown event type. */
        Earthquake,        /*!< Earthquake. */
        QuarryBlast,       /*!< Quarry blast. */
        SubnetTrigger,     /*!< Subnet trigger. */
        MiningInduced,     /*!< Mining induced. */
        Explosion,         /*!< Regular explosion. */
        ChemicalExplosion, /*!< Chemical explosion. */
        NuclearExplosion,  /*!< Nuclear explosion. */
        NuclearTest,       /*!< Nuclear test. */
        Sonic,             /*!< Sonic. */
        Avalanche,         /*!< Avalanche. */ 
        Collapse,          /*!< (Mining) collapse. */
        Landslide          /*!< Landslide. */
    }; 
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Event();
    /// @brief Copy constructor.
    /// @param[in] event  The event class from which to initialize this class.
    Event(const Event &event);
    /// @brief Move constructor.
    /// @param[in,out] event  The event class from which to initialize this 
    ///                       class.  On exit, event's behavior is undefined.
    Event(Event &&event) noexcept;
    /// @}

    /// @name Properties Required by the Database
    /// @{

    /// @brief Sets the event identifier.
    /// @param[in] identifier  The event identifier.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The event identifier.
    /// @throws std::runtime_error if \c haveIdentifier() is false.
    [[nodiscard]] int64_t getIdentifier() const;
    /// @result True indicates the event identifier was set.
    [[nodiscard]] bool haveIdentifier() const noexcept;

    /// @brief Sets the authority that created the event.
    /// @param[in] authority   The authority that created the event.
    void setAuthority(const std::string &authority);
    /// @result The authority that created the event.
    /// @throws std::runtime_error if \c haveAuthority() is false.
    [[nodiscard]] std::string getAuthority() const;
    /// @result True indicates the authority was set.
    [[nodiscard]] bool haveAuthority() const noexcept;

    /// @brief Sets the event version number.
    /// @param[in] version  The event version number.
    /// @note This is incremented each time the preferred origin, preferred
    ///       magnitude, or preferred mechanism is updated.
    void setVersion(uint16_t version) noexcept;
    /// @result The event's version number.
    /// @throws std::runtime_error if \c haveVersion() is false.
    [[nodiscard]] int getVersion() const;
    /// @result True indicates the version number was set.
    [[nodiscard]] bool haveVersion() const noexcept;
    /// @}

    /// @name Optional Parameters
    /// @{

    /// @brief Sets the preferred origin identifier.
    /// @param[in] identifier  The identifier of the origin preferred by this
    ///                        event.
    void setPreferredOriginIdentifier(int64_t identifier) noexcept;
    /// @result The identifier of the preferred origin.
    [[nodiscard]] std::optional<int64_t> getPreferredOriginIdentifier() const noexcept; 

    /// @brief Sets the preferred magnitude identifier.
    /// @param[in] identifier  The identifier of the magnitude preferred by this
    ///                        event.
    void setPreferredMagnitudeIdentifier(int64_t identifier) noexcept;
    /// @result The identifier of the preferred magnitude.
    [[nodiscard]] std::optional<int64_t> getPreferredMagnitudeIdentifier() const noexcept; 

    /// @brief Sets the preferred mechanism identifier.
    /// @param[in] identifier  The identifier of the mechanism preferred by this
    ///                        event.
    void setPreferredMechanismIdentifier(int64_t identifier) noexcept;
    /// @result The identifier of the preferred mechanism.
    [[nodiscard]] std::optional<int64_t> getPreferredMechanismIdentifier() const noexcept; 

    /// @brief Sets the comment identifier.
    /// @param[in] identifier  The comment identifier.
    void setCommentIdentifier(int64_t identifier) noexcept;
    /// @result The comment identifier.
    [[nodiscard]] std::optional<int64_t> getCommentIdentifier() const noexcept;

    /// @brief Sets a secondary identifier to indicate where the origin
    ///        was originally created.
    /// @param[in] subsource   The subsource - e.g., Jiggle or RT1.
    void setSubSource(const std::string &subsource);
    /// @result The secondary identifier indicating where the origin was 
    ///         originally created.
    [[nodiscard]] std::optional<std::string> getSubSource() const noexcept;

    /// @brief Sets the event type.
    /// @param[in] type  The event type.
    void setType(Type type) noexcept;
    /// @result The event type.
    [[nodiscard]] std::optional<Type> getType() const noexcept;

    /// @brief Sets the event as definitive.
    void setSelectFlag() noexcept;
    /// @brief Sets the event as non-definitive.
    void unsetSelectFlag() noexcept; 
    /// @result True indicates this is the definitive solution.
    [[nodiscard]] bool getSelectFlag() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~Event();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] event  The event to copy to this.
    /// @result A deep copy of the event.
    Event& operator=(const Event &event);
    /// @brief Move assignment.
    /// @param[in,out] event  The event whose memory will be moved to this.
    ///                       On exit, event's behavior is undefined.
    /// @result The memory from event moved to this.
    Event& operator=(Event &&event) noexcept;
    /// @}
private:
    class EventImpl;
    std::unique_ptr<EventImpl> pImpl;
};
[[nodiscard]] std::string toInsertString(const Event &event);
}
#endif

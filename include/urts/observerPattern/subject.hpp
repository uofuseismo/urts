#ifndef URTS_OBSERVER_PATTERN_SUBJECT_HPP
#define URTS_OBSERVER_PATTERN_SUBJECT_HPP
#include <memory>
#include "urts/observerPattern/enums.hpp"
namespace URTS::ObserverPattern
{
class IObserver;
/// @class ISubject "subject.hpp" "urts/observerPattern/subject.hpp"
/// @brief Abstract base class for subjects.  Typically, subjects will be
///        (data) models which widgets (observers) will monitor and respond
///        to changes.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @date January 14 2021
class ISubject
{
public:
    /// @brief Constructor.
    ISubject();
    /// @brief Destructor.
    ~ISubject();
    /// @brief Interface for an observer to subscribe to a subject.
    /// @param[in] message   The message type that interests the observer.
    /// @param[in] observer  A pointer to the observer class.  This will allow
    ///                      the subject to call the observer's update method.
    virtual void subscribe(Message message, IObserver *observer);
    /// @brief Allows an observer to unsubscribe from the subject and message
    ///        type.
    /// @param[in] message   The message type that the observer was monitoring.
    /// @param[in] observer  A pointer to the observer class.
    virtual void unsubscribe(Message message, IObserver *observer);
    /// @brief Allows
    /// @brief Notifies the observers of a change by calling the observer's
    ///        update method.
    virtual void notify(Message message);
private:
    class ISubjectImpl;
    std::unique_ptr<ISubjectImpl> pImpl;
};
}
#endif // URTS_OBSERVER_PATTERN_SUBJECT_HPP

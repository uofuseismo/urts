#ifndef URTS_OBSERVER_PATTERN_OBSERVER_HPP
#define URTS_OBSERVER_PATTERN_OBSERVER_HPP
#include "urts/observerPattern/enums.hpp"
namespace URTS::ObserverPattern
{
/// @class IObserver "observer.hpp" "urts/observerPattern/observer.hpp"
/// @brief An abstract base class for observers.  In short, observers monitor a
///        subject (data model).  When the model is updated the subject will
///        issue an `update' command.  Each observer must implement the method
///        updating.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @date January 14 2021
class IObserver
{
public:
    /// @brief Destructor.
    virtual ~IObserver() = default;
    /// @brief The mechanism by which each observer updates itself in response
    ///        to a change in its subject.
    virtual void updateObserver(URTS::ObserverPattern::Message message) = 0;
};
}
#endif //URTS_OBSERVERPATTERN_OBSERVER_HPP

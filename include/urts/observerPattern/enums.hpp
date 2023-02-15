#ifndef URTS_OBSERVER_PATTERN_ENUMS_HPP
#define URTS_OBSERVER_PATTERN_ENUMS_HPP
namespace URTS::ObserverPattern
{
/// @class Message "message.hpp" "urts/observerPattern/message.hpp"
/// @brief Defines the message type in the observer pattern.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @date January 14 2021
enum class Message
{
    Update,   /*!< Indicates that the subject as been updated. */
    NoChange  /*!< Indicates that the subject was intended to be updated
                   however there was ultimately no change. */
};
}
#endif //URTS_OBSERVER_PATTERN_ENUMS_HPP

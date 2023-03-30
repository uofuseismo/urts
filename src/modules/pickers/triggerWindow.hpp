#ifndef URTS_MODULES_PICKERS_TRIGGER_WINDOW_HPP
#define URTS_MODULES_PICKERS_TRIGGER_WINDOW_HPP
#include <memory>
#include <chrono>
namespace URTS::Modules::Pickers
{
/// @class TriggerWindow "triggerWindow.hpp" "urts/modules/pickers/triggerWindow.hpp"
/// @brief Defines a trigger window.  A trigger window begins when the
///        characteristic function exceeds a predefined threshold then
///        falls below another predefined threshold.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
template<class T = double>
class TriggerWindow
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    TriggerWindow();
    /// @brief Copy constructor.
    /// @param[in] window  The window class from which to initialize this class.
    TriggerWindow(const TriggerWindow &window); 
    /// @brief Move constructor.
    /// @param[in,out] window  The window class from which to initialize this
    ///                         class.  On exit, window's behavior is undefined.
    TriggerWindow(TriggerWindow &&window) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @result A deep copy of the input window.
    TriggerWindow& operator=(const TriggerWindow &window);
    /// @brief Move assignment.
    /// @param[in,out] window  The window class whose memory will be moved
    ///                        to this.  On exit, window's behavior is
    ///                        undefined.
    /// @result The memory from window moved to this. 
    TriggerWindow& operator=(TriggerWindow &&window) noexcept;
    /// @}

    /// @name Start
    /// @{

    /// @brief Sets the time and value of the first sample in the
    ///        trigger window.
    /// @param[in] start  The time in UTC microseconds since the epoch and
    ///                   value of the first sample of the trigger window.
    void setStart(const std::pair<std::chrono::microseconds, T> &start) noexcept;
    /// @result The time and value of the first sample of the trigger window.
    /// @throws std::invalid_argument if \c haveStart() is false.
    [[nodiscard]] std::pair<std::chrono::microseconds, T> getStart() const;
    /// @result True indicates the initial sample of the trigger window
    ///         was set.
    [[nodiscard]] bool haveStart() const noexcept;
    /// @}

    /// @name End
    /// @{

    /// @brief Sets the time and value of the final sample in the
    ///        trigger window.
    /// @param[in] end  The time in UTC microseconds since the epoch and
    ///                 value of the final sample of the trigger window.
    void setEnd(const std::pair<std::chrono::microseconds, T> &end) noexcept;
    /// @result The time and value of the final sample of the trigger window.
    /// @throws std::invalid_argument if \c haveEnd() is false.
    [[nodiscard]] std::pair<std::chrono::microseconds, T> getEnd() const;
    /// @result True indicates the terminal sample of the trigger window
    ///         was set.
    [[nodiscard]] bool haveEnd() const noexcept;
    /// @}

    /// @name Maximum
    /// @{

    /// @brief Sets the value and time of the largest sample in the
    ///        trigger window.
    /// @param[in] maximum  The time in UTC microseconds since the epoch
    ///                     and value of the largest sample.
    void setMaximum(const std::pair<std::chrono::microseconds, T> &maximum) noexcept;
    /// @result The time and value of the largest sample in the trigger window.
    /// @throws std::runtime_error if \c haveMaximumValue() is false.
    [[nodiscard]] std::pair<std::chrono::microseconds, T> getMaximum() const;
    /// @result True indicates the maximum value was set.
    [[nodiscard]] bool haveMaximum() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~TriggerWindow();
    /// @}
private:
    class TriggerWindowImpl;
    std::unique_ptr<TriggerWindowImpl> pImpl;
};
}
#endif

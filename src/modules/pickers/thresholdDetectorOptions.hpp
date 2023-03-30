#ifndef URTS_MODULES_PICKERS_THRESHOLD_DETECTOR_OPTIONS_HPP
#define URTS_MODULES_PICKERS_THRESHOLD_DETECTOR_OPTIONS_HPP
#include <memory>
#include <chrono>
namespace URTS::Modules::Pickers
{
/// @class DetectorOptions "detectorOptions.hpp" "urts/modules/pickers/thresholdDetectorOptions.hpp"
/// @brief Defines the options for the threshold detector.  The threshold 
///        detector identifies trigger windows.  Here, trigger windows are
///        windows that begin when a characteristic function (or posterior
///        probability time series) exceeds a threshold.  The window terminates
///        when the characteristic function falls below another threshold.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ThresholdDetectorOptions
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ThresholdDetectorOptions();
    /// @brief Copy constructor.
    /// @param[in] options  The threshold detector options from which to
    ///                     initialize this class.
    ThresholdDetectorOptions(const ThresholdDetectorOptions &options);
    /// @brief Move constructor.
    /// @param[in,out] options  The threshold detector options from which to
    ///                         initialize this class.  On exit, threshold's
    ///                         behavior is undefined.
    ThresholdDetectorOptions(ThresholdDetectorOptions &&options) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] options  The detector options to copy to this.
    /// @result A deep copy of the input detector options.
    ThresholdDetectorOptions& operator=(const ThresholdDetectorOptions &options);
    /// @brief Move assignment.
    /// @param[in,out] options  The detector options whose memory will be moved
    ///                         to this.  On exit, options's behavior is
    ///                         undefined.
    /// @result The memory from the detector options moved to this.
    ThresholdDetectorOptions& operator=(ThresholdDetectorOptions &&options) noexcept;
    /// @}

    /// @brief Sets the value that the characteristic function must exceed to
    ///        begin a trigger window. 
    void setOnThreshold(double threshold) noexcept;
    /// @result The on threshold.
    /// @throws std::runtime_error if \c haveOnThreshold() is false.
    [[nodiscard]] double getOnThreshold() const;
    /// @result True indicates the on threshold was set.
    [[nodiscard]] bool haveOnThreshold() const noexcept;

    /// @brief Sets the value that the characteristic function must fall below
    ///        to terminate the trigger window.
    void setOffThreshold(double threshold) noexcept;
    /// @result The off threshold.
    /// @throws std::runtime_error if \c haveOffThreshold() is false.
    [[nodiscard]] double getOffThreshold() const;
    /// @result True indicates the off treshold was set.
    [[nodiscard]] bool haveOffThreshold() const noexcept;

    /// @brief Sets the gap duration.
    /// @param[in] nSamples  If the amount of samples between successive packets
    ///                      exceeds this value then a gap is declared and the
    ///                      detector is reset.
    /// @throws std::invalid_argument if this is negative.
    void setMinimumGapSize(int nSamples);
    /// @result The gap duration.
    [[nodiscard]] int getMinimumGapSize() const noexcept;

    /// @brief The maximum trigger duration.  This is effectively a safety
    ///        mechanism that prevents the trigger from getting indefinitely
    ///        stuck in the on position.
    /// @param[in] duration  If the trigger window is for greater than this
    ///                      time then the trigger window is terminated and
    ///                      the trigger discarded.  This can be disabled by
    ///                      setting a non-positive number.
    void setMaximumTriggerDuration(const std::chrono::microseconds &duration) noexcept; 
    /// @result The maximum trigger duration.
    [[nodiscard]] std::chrono::microseconds getMaximumTriggerDuration() const noexcept;

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~ThresholdDetectorOptions();
    /// @}
private:
    class ThresholdDetectorOptionsImpl;
    std::unique_ptr<ThresholdDetectorOptionsImpl> pImpl;
};
}
#endif

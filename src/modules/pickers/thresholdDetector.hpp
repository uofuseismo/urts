#ifndef URTS_MODULES_PICKERS_THRESHOLD_DETECTOR_HPP
#define URTS_MODULES_PICKERS_THRESHOLD_DETECTOR_HPP
#include <memory>
#include <chrono>
namespace URTS::Broadcasts::Internal
{
 namespace DataPacket
 {
  class DataPacket;
 }
 namespace ProbabilityPacket
 {
  class ProbabilityPacket;
 }
}
namespace URTS::Modules::Pickers
{
template<class T> class TriggerWindow;
class ThresholdDetectorOptions;
}
namespace URTS::Modules::Pickers
{
/// @class ThresholdDetector "thresholdDetector.hpp" "urts/modules/pickers/thresholdDetector.hpp"
/// @brief Defines a threshold detector.  This effectively identifies
///        trigger windows where a characteristic function (or posterior 
///        probability time series) exceeds and then falls below another
///        threshold.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ThresholdDetector
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ThresholdDetector();
    /// @brief Copy constructor.
    /// @param[in] detector  The threshold detector from which to initialize
    ///                      this class.
    ThresholdDetector(const ThresholdDetector &detector);
    /// @brief Move constructor.
    /// @param[in,out] detector  The threshold detector from which to initialize
    ///                          this class.  On exit, threshold's behavior is
    ///                          undefined.
    ThresholdDetector(ThresholdDetector &&detector) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] detector  The detector to copy to this.
    /// @result A deep copy of the input detector.
    ThresholdDetector& operator=(const ThresholdDetector &detector);
    /// @brief Move assignment.
    /// @param[in,out] detector  The detector whose memory will be moved to
    ///                          this.  On exit, detector's behavior is
    ///                          undefined.
    /// @result The memory from the detector moved to this.
    ThresholdDetector& operator=(ThresholdDetector &&detector) noexcept;
    /// @}

    /// @brief Initializes the detector.
    /// @param[in] options  The threshold detector options.
    /// @throws std::invalid_argument if the on and off tolerances are not set.
    void initialize(const ThresholdDetectorOptions &options);
    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;
 
    /// @brief Sets the initial conditions.
    /// @throws std::runtime_error if \c isInitialized() is false.
    void setInitialConditions();

    /// @brief Applies the threshold detector to a data packet.
    /// @param[in] packet  The data packet on which to run the threshold
    ///                    detector.
    /// @throws std::invalid_argument if the packet does not have a 
    ///         sampling rate.
    /// @throws std::runtime_error if \c isInitialized() is false.
    void apply(const URTS::Broadcasts::Internal::DataPacket::DataPacket &packet,
               std::vector<TriggerWindow<double>> *triggerWindows);
    /// @brief Applies the threshold detector to a posterior probability packet.
    /// @param[in] packet  The posterior probability packet on which to run
    ///                    the threshold detector.
    /// @throws std::invalid_argument if the packet does not have a 
    ///         sampling rate.
    /// @throws std::runtime_error if \c isInitialized() is false.
    void apply(const URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket &packet,
               std::vector<TriggerWindow<double>> *triggerWindows);
    /// @brief Applies the threshold detector the given characteristic function
    ///        signal (or posterior probability signal.
    /// @param[in] signal        The signal from which to apply the threshold
    ///                          detector.
    /// @param[in] samplingRate  The sampling rate of the signal snippet in Hz. 
    /// @param[in] startTime     The start time (UTC) of the signal in microseconds
    ///                          since the epoch.
    /// @throws std::invalid_argument if the sampling rate is not positive or
    ///         triggerWindows is NULL.
    void apply(const std::vector<double> &signal,
               const double samplingRate,
               const std::chrono::microseconds &startTime,
               std::vector<TriggerWindow<double>> *triggerWindows);
    /// @brief Applies the detector.
    /// @param[in] samplingRate     The packet's sampling rate. 
    /// @param[in] startTime        The packet's start time in microseconds since
    ///                             the epoch.
    /// @param[in] nSamples         The number of samples in the packet.
    /// @param[in] signal           The signal in the packet.
    /// @param[out] triggerWindows  The list of triggers.  If
    ///                             triggerWindows->empty() is true then there
    ///                             are currently no triggers.
    /// @throws std::invalid_argument if the sampling rate is not positive,
    ///         nSamples is greater than zero and the signal is null, or
    ///         triggerWindows is null.
    /// @throws std::runtime_error if \c isInitialized() is false.
    //void apply(double samplingRate,
//               const std::chrono::microseconds &startTime,
//               int nSamples,
//               const T *signal, 
//               std::vector<TriggerWindow<T>> *triggerWindows);
    /// @brief Resets the class to its initial conditions.
    /// @throws std::runtime_error if \c isIntiialized() is false.
    void resetInitialConditions();

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~ThresholdDetector();
    /// @}
private:
    class ThresholdDetectorImpl;
    std::unique_ptr<ThresholdDetectorImpl> pImpl;
};
}
#endif

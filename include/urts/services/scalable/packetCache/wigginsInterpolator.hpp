#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_WIGGINS_INTERPOLATOR_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_WIGGINS_INTERPOLATOR_HPP
#include <memory>
#include <chrono>
#include <vector>
namespace URTS::Broadcasts::Internal::DataPacket
{
 class DataPacket;
}
namespace URTS::Services::Scalable::PacketCache
{
/// @class WigginsInterpolator "wigginsInterpolator.hpp" "urts/services/scalable/packetCache/wigginsInterpolator.hpp"
/// @brief Performs Wiggins interpolation on a vector of data packets. 
///        This can heal the packets returned by a packet cache query.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class WigginsInterpolator
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    WigginsInterpolator();
    /// @brief Copy constructor.
    /// @param[in] wiggins  The interpolator from which to initialize this
    ///                     class. 
    WigginsInterpolator(const WigginsInterpolator &wiggins);
    /// @brief Move constructor.
    /// @param[in,out] wiggins  The interpolator from which to initialize this
    ///                         class.  On exit, wiggins's behavior is
    ///                         undefined.
    WigginsInterpolator(WigginsInterpolator &&wiggins) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assigment.
    /// @param[in] wiggins  The interpolator class to copy to this.
    /// @result A deep copy of the wiggins interpolator.
    WigginsInterpolator& operator=(const WigginsInterpolator &wiggins);
    /// @brief Move assignment.
    /// @param[in,out] wiggins  The class whose memory will be moved to this.
    ///                         On exit, wiggins's behavior is undefined.
    /// @result the memory form wiggins moved to this.
    WigginsInterpolator& operator=(WigginsInterpolator &&wiggins) noexcept;
    /// @}

    /// @name Target Sampling Rate
    /// @{

    /// @brief Sets the target sampling rate.  This is the sampling rate to
    ///        which signals will be interpolated.
    /// @param[in] samplingRate  The sampling rate, in Hz, to which the packets
    ///                          will be interpolated. 
    /// @throws std::invalid_argument if this is not positive.
    void setTargetSamplingRate(double samplingRate);
    /// @result The nominal sampling ratein Hz.  By default this is 100 Hz.
    [[nodiscard]] double getTargetSamplingRate() const noexcept;
    /// @}

    /// @name Gap Tolerance
    /// @{

    /// @brief Sets the gap tolerance between packets.
    /// @param[in] tolerance  If the time between the end of a packet and the
    ///                       start of the next packet exceeds this time
    ///                       then the samples interpolated between the packets
    ///                       will be denoted as extrapolated data gaps.
    /// @note Zero or negative effectively disables this.  In which case any
    ///       sample interpolated between a packet is being extrapolated in a 
    ///       gap.
    void setGapTolerance(const std::chrono::microseconds &tolerance) noexcept;
    /// @result The gap tolerance.  By default this is 30000 (0.03 seconds)
    ///         or 3 samples at the nominal sampling rate of 100 Hz.
    [[nodiscard]] std::chrono::microseconds getGapTolerance() const noexcept;
    /// @}

    /// @name Interpolate
    /// @{

    /// @brief Interpolates the data packets.  This will interpolate from the
    ///        earliest time in the colleciton of packets up to the latest
    ///        time in the packets.
    /// @param[in] packets    The packets to interpolate.
    /// @param[in] startTime  The start time (UTC) in microseconds since the
    ///                       epoch of the interpolation.
    ///                       If this is less than the minimum start time
    ///                       of all the packets then the interpolation will
    ///                       start at the minimum start time of all packets.
    /// @param[in] endTime    The end time (UTC) in microseconds since the
    ///                       epoch of the interpolation.
    ///                       If this is greater than the maximum end time
    ///                       of all the packets then the interpolation will
    ///                       end at the maximum end time of all packets.
    /// @throws std::runtime_error if an error occurs.
    /// @throws std::invalid_argument if the start time exceeds the end time.
    void interpolate(const std::vector<URTS::Broadcasts::Internal::DataPacket::DataPacket> &packets,
                     const std::chrono::microseconds &startTime = std::chrono::microseconds {-631152000000000},
                     const std::chrono::microseconds &endtime   = std::chrono::microseconds {5680281600000000});
    /// @brief Interpolates the data packets.
    /// @param[in] nPackets   The number of packets (which must be positive).
    /// @parma[in] packets    An array of data packets.  This has dimension
    ///                       [nPackets].
    /// @param[in] startTime  The start time (UTC) in microseconds since the
    ///                       epoch of the interpolation. 
    /// @param[in] endTime    The end time (UTC) in microsecond since the
    ///                       epoch of the interpolation.
    /// @throws std::invalid_argument if the start time exceeds the end time,
    ///         the number of packets is positive and packets is NULL. 
    void interpolate(int nPackets, const URTS::Broadcasts::Internal::DataPacket::DataPacket packets[],
                     const std::chrono::microseconds &startTime = std::chrono::microseconds {-631152000000000},
                     const std::chrono::microseconds &endtime   = std::chrono::microseconds {5680281600000000});
    /// @}

    /// @name Interpolated Signal
    /// @{
   
    /// @result The interpolated signal.
    [[nodiscard]] std::vector<double> getSignal() const noexcept;
    /// @result A pointer to the interpolated signal.  This is an array whose
    ///         dimension is [\c getNumberOfSamples].
    /// @note This exists for performance reasons.  When possible use
    ///       \c getSignal().
    [[nodiscard]] const double *getSignalPointer() const noexcept;
    /// @result A pointer to the interpolated signal.  This is an array whose
    ///         dimension is [\c getNumberOfSamples].
    /// @note This exists for performance reasons.  When possible use
    ///       \c getSignal().
    [[nodiscard]] const std::vector<double> &getSignalReference() const noexcept;
    /// @result The number of samples in the interpolated signal.
    [[nodiscard]] int getNumberOfSamples() const noexcept;

    /// @result An array of true/false that indicates whether or not the 
    ///         interpolated signal fell between packet endpoints.   
    [[nodiscard]] std::vector<int8_t> getGapIndicator() const noexcept;
    /// @result A pointer to a signal that indicates whether or not the
    ///         sample in the interpolated signal was within a packet (false) or
    ///         between packets (true) - i.e., extrapolated.
    /// @note This exists for performance reasons.  When possibleu se
    ///       \c getGapIndicator().
    [[nodiscard]] const int8_t *getGapIndicatorPointer() const noexcept;
    /// @result A pointer to a signal that indicates whether or not the
    ///         sample in the interpolated signal was within a packet (false) or
    ///         between packets (true) - i.e., extrapolated.
    /// @note This exists for performance reasons.  When possibleu se
    ///       \c getGapIndicator().
    [[nodiscard]] const std::vector<int8_t> &getGapIndicatorReference() const noexcept;
    /// @result True indicates that there are non-zeros in the gapIndicator.
    [[nodiscard]] bool haveGaps() const noexcept;

    /// @result The start time of the interpolated signal.
    [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
    /// @result The end time of the interpolated signal.
    [[nodiscard]] std::chrono::microseconds getEndTime() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases all memory.
    void clear() noexcept;
    /// @brief Resets the start/end time to zero and releases memory of the
    ///        gap pointer and signal.  This will not modify the gap tolerance
    ///        or target sampling rate.
    void clearSignal() noexcept; 
    /// @brief Destructor.
    ~WigginsInterpolator();
    /// @}
private:
    class WigginsInterpolatorImpl;
    std::unique_ptr<WigginsInterpolatorImpl> pImpl;
};
}
#endif

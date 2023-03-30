#ifndef URTS_BROADCASTS_INTERNAL_PROBABILITY_PACKET_DATA_PACKET_HPP
#define URTS_BROADCASTS_INTERNAL_PROBABILITY_PACKET_DATA_PACKET_HPP
#include <vector>
#include <chrono>
#include <memory>
#include <umps/messageFormats/message.hpp>
namespace URTS::Broadcasts::Internal::ProbabilityPacket
{
/// @class ProbabilityPacket "probabilityPacket.hpp" "urts/broadcasts/internal/probabilityPacket/probabilityPacket.hpp"
/// @brief Defines a probability packet. 
/// @note This is for binary classification algorithms.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
/// @ingroup Modules_Broadcasts_Internal_ProbabilityPacket
class ProbabilityPacket : public UMPS::MessageFormats::IMessage
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ProbabilityPacket();
    /// @brief Copy constructor.
    /// @param[in] packet  The data packet from which to initialize this class.
    ProbabilityPacket(const ProbabilityPacket &packet);
    /// @brief Move constructor.
    /// @param[in,out] packet  The data packet from which to initialize this
    ///                        class.  On exit, packet's behavior is undefined.
    ProbabilityPacket(ProbabilityPacket &&packet) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] packet  The packet to copy to this class.
    /// @result A deep copy of the input packet.
    ProbabilityPacket& operator=(const ProbabilityPacket &packet);
    /// @brief Move assignment.
    /// @param[in,out] packet  The packet whose memory will be moved to
    ///                        this class.
    /// @result The memory from packet moved to this.
    ProbabilityPacket& operator=(ProbabilityPacket &&packet) noexcept; 
    /// @}
 
    /// @name Required Information
    /// @{

    /// @brief Sets the network code.
    /// @param[in] network  The network code.
    /// @throws std::invalid_argument if network is empty.
    void setNetwork(const std::string &network);
    /// @result The network code.
    /// @throws std::runtime_error if \c haveNetwork() is false.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates that the network was set.
    [[nodiscard]] bool haveNetwork() const noexcept;

    /// @brief Sets the station name.
    /// @param[in] station   The station name.
    /// @throws std::invalid_argument if station is empty.
    void setStation(const std::string &station);
    /// @result The station name.
    /// @throws std::runtime_error if \c haveStation() is false.
    [[nodiscard]] std::string getStation() const;
    /// @result True indicates that the station name was set.
    [[nodiscard]] bool haveStation() const noexcept;

    /// @brief Sets the channel name.
    /// @param[in] channel  The channel name.
    /// @throws std::invalid_argument if channel is empty.
    void setChannel(const std::string &channel);
    /// @result The channel name.
    /// @throws std::runtime_error if the channel was not set.
    [[nodiscard]] std::string getChannel() const;
    /// @result True indicates that the channel was set.
    [[nodiscard]] bool haveChannel() const noexcept;

    /// @brief Sets the location code.
    /// @param[in] location  The location code.
    /// @throws std::invalid_argument if location is empty.
    void setLocationCode(const std::string &location);
    /// @brief Sets the location code.
    /// @throws std::runtime_error if \c haveLocationCode() is false.
    [[nodiscard]] std::string getLocationCode() const;
    /// @result True indicates that the location code was set.
    [[nodiscard]] bool haveLocationCode() const noexcept;

    /// @brief Sets the sampling rate for data in the packet.
    /// @param[in] samplingRate  The sampling rate in Hz.
    /// @throws std::invalid_argument if samplingRate is not positive.
    void setSamplingRate(double samplingRate);
    /// @result The sampling rate of the packet in Hz.
    /// @throws std::runtime_error if \c haveSamplingRate() is false.
    [[nodiscard]] double getSamplingRate() const;
    /// @result True indicates that the sampling rate was set.
    [[nodiscard]] bool haveSamplingRate() const noexcept; 
    /// @}

    /// @name Optional Information
    /// @{

    /// @brief The probability pickers convert multiple input channels to a
    ///        single probability stream.
    /// @param[in] originalChannels  The original channels that were converted
    ///                              to a probability signal.  For three
    ///                              component data this could be
    ///                              "HHZ", "HHN", "HHE".  For one component
    ///                              data this could be "HHZ".
    void setOriginalChannels(const std::vector<std::string> &originalChannels) noexcept; 
    /// @result The original channels that were converted to a probability
    ///         signal.
    [[nodiscard]] std::vector<std::string> getOriginalChannels() const noexcept;

    /// @param[in] startTime  The UTC start time in seconds from the epoch
    ///                       (Jan 1, 1970).
    void setStartTime(double startTime) noexcept;
    /// @param[in] startTime  The UTC start time in microseconds
    ///                       from the epoch (Jan 1, 1970). 
    void setStartTime(const std::chrono::microseconds &startTime) noexcept;
    /// @result The UTC start time in microseconds from the epoch.
    [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
    //[[nodiscard]] int64_t getStartTime() const noexcept;
    /// @result The UTC time in microseconds from the epoch of the last sample.
    /// @throws std::runtime_error if \c haveSamplingRate() is false or
    ///         \c getNumberOfSamples() is 0.
    [[nodiscard]] std::chrono::microseconds getEndTime() const;

    /// @brief Sets the machine learning algorithm details.
    /// @param[in] algorithm  The machine learning algorithm - e.g.,
    ///                       UNetThreeComponentP.
    void setAlgorithm(const std::string &algorithm) noexcept;
    /// @result The machine learning algorithm.
    [[nodiscard]] std::string getAlgorithm() const noexcept;

    /// @brief Sets the name of the positive class.  When the probability signal
    ///        is large then this class becomes more likely.
    /// @param[in] positiveClass   The name of the postiive class -
    ///                            e.g., "P" or "S".
    void setPositiveClassName(const std::string &positiveClass) noexcept;
    /// @result The name of the positive class.
    [[nodiscard]] std::string getPositiveClassName() const noexcept;

    /// @brief Sets the name of the negative class.  When the probability signal
    ///        is small then this class becomes more likely.
    /// @param[in] negativeClass   The name of the negative class -
    ///                            e.g., "Noise".
    void setNegativeClassName(const std::string &negativeClass) noexcept;
    /// @result The name of the negative class.
    [[nodiscard]] std::string getNegativeClassName() const noexcept;
    /// @}

    /// @name Data
    /// @{

    /// @brief Sets the probability time series data in this packet.
    /// @param[in] data  The probability time series data.
    template<typename U>
    void setData(const std::vector<U> &data) noexcept;
    /// @brief Sets the probability time series data in this packet.
    /// @param[in,out] data  The probability time series data whose memory 
    ///                      will be moved into this class.  On exit, data's
    ///                      behavior is undefined.
    void setData(std::vector<double> &&data) noexcept;
    /// @result The probability time series currently set on the packet. 
    [[nodiscard]] std::vector<double> getData() const noexcept;
    /// @result A reference to the probability time series currently set on
    ///         the packet.
    [[nodiscard]] const std::vector<double> &getDataReference() const noexcept;
    /// @result The number of data samples in the packet.
    [[nodiscard]] int getNumberOfSamples() const noexcept;
    /// @}

    /// @name Message Abstract Base Class Properties
    /// @{

    /// @result A copy of this class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> clone() const final;
    /// @result An instance of an uninitialized class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> createInstance() const noexcept final;
    /// @brief Converts the packet class to a string message.
    /// @result The class expressed as a string message.
    /// @throws std::runtime_error if the required information is not set. 
    /// @note Though the container is a string the message need not be
    ///       human readable.
    [[nodiscard]] std::string toMessage() const final;
    /// @brief Creates the class from a message.
    void fromMessage(const std::string &message) final;
    /// @brief Creates the class from a message.
    /// @param[in] data    The contents of the message.  This is an
    ///                    array whose dimension is [length] 
    /// @param[in] length  The length of data.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if data is NULL or length is 0. 
    void fromMessage(const char *data, size_t length) final;
    /// @result The message type - e.g., "ProbabilityPacket".
    [[nodiscard]] std::string getMessageType() const noexcept final;
    /// @result The message version.
    [[nodiscard]] std::string getMessageVersion() const noexcept final;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases all memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~ProbabilityPacket() override;
    /// @}
private:
    class ProbabilityPacketImpl;
    std::unique_ptr<ProbabilityPacketImpl> pImpl;
};
}
#endif

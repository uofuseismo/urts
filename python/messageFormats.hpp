#ifndef URTS_PYTHON_MESSAGE_FORMATS_HPP
#define URTS_PYTHON_MESSAGE_FORMATS_HPP
#include <memory>
#include <chrono>
#include <umps/messageFormats/message.hpp>
//#include <python/messaging.hpp>
//#include <python/logging.hpp>
#include <python/messageFormats.hpp>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
namespace URTS::MessageFormats
{
 class DataPacket;
}
namespace URTS::Python::MessageFormats
{
/// @brief Wraps a data packet.
class DataPacket : public UMPS::Python::MessageFormats::IMessage
{
public:
    /// @brief Constructor.
    DataPacket();
    /// @brief Copy constructor.
    DataPacket(const DataPacket &packet);
    /// @brief Constructs from the native class.
    DataPacket(const URTS::MessageFormats::DataPacket &packet);
    /// @brief Move constructor.
    DataPacket(DataPacket &&packet) noexcept;
    /// @brief Copy assignment operator.
    DataPacket& operator=(const DataPacket &packet);
    /// @brief Copy from native class.
    DataPacket& operator=(const URTS::MessageFormats::DataPacket &packet);
    /// @brief Move assignment operator.
    DataPacket& operator=(DataPacket &&packet) noexcept;
    /// @result A reference to the native class.
    [[nodiscard]] const URTS::MessageFormats::DataPacket& getNativeClassReference() const noexcept;
    /// @result A copy of the native class.
    [[nodiscard]] URTS::MessageFormats::DataPacket getNativeClass() const noexcept;
    /// @brief An instance of this class.
    [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> createInstance() const override;
    /// @brief A copy of this class.
    [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> clone(const std::unique_ptr<UMPS::MessageFormats::IMessage> &message) const override;
    /// @brief A clone of the base class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> getInstanceOfBaseClass() const noexcept override;
    /// @brief Constructs from the base class.
    void fromBaseClass(UMPS::MessageFormats::IMessage &message) override;
    /// @result The message type.
    [[nodiscard]] std::string getMessageType() const noexcept override;
    /// @brief Sets the network code.
    void setNetwork(const std::string &network);
    /// @result Network code.
    [[nodiscard]] std::string getNetwork() const;
    /// @brief Sets the station code.
    void setStation(const std::string &station);
    /// @result Network code.
    [[nodiscard]] std::string getStation() const;
    /// @brief Sets the channel code.
    void setChannel(const std::string &channel);
    /// @result Channel code.
    [[nodiscard]] std::string getChannel() const;
    /// @brief Sets the location code.
    void setLocationCode(const std::string &location);
    /// @result Location code.
    [[nodiscard]] std::string getLocationCode() const;
    /// @brief Sampling rate
    void setSamplingRate(const double samplingRate);
    /// @result Sampling rate.
    [[nodiscard]] double getSamplingRate() const;
    /// @brief Sets the start time.
    void setStartTime(const std::chrono::microseconds &time) noexcept;
    /// @result The start time.
    [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
    /// @result Number of samples.
    [[nodiscard]] int getNumberOfSamples() const noexcept;
    /// @brief Sets the data.
    void setData(const pybind11::array_t<double, pybind11::array::c_style | pybind11::array::forcecast> &x); 
    /// @result The data.
    [[nodiscard]] std::vector<double> getData() const noexcept;
    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~DataPacket() override;
private:
    std::unique_ptr<URTS::MessageFormats::DataPacket> pImpl{nullptr};
};

/// 
void initialize(pybind11::module &m);
}
#endif

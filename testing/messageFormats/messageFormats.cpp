#include <string>
#include <cmath>
#include <cstring>
#include <vector>
#include <chrono>
#include <limits>
#include "urts/messageFormats/dataPacket.hpp"
#include <gtest/gtest.h>
namespace
{

using namespace URTS::MessageFormats;

TEST(MessageFormats, DataPacket)
{
    const std::string messageType{"URTS::MessageFormats::DataPacket"};
    const std::string network{"UU"};
    const std::string station{"FORK"};
    const std::string channel{"HHZ"};
    const std::string locationCode{"01"};
    const std::vector<double> timeSeries{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const double startTime{1628803598};
    const std::chrono::microseconds startTimeMuS{1628803598000000};
    const std::chrono::microseconds endTimeMuS{1628803598000000 + 225000}; // 225000 = std::round(9./40*1000000
    double samplingRate{40};
    auto tol = std::numeric_limits<double>::epsilon();

    DataPacket dataPacket;
    dataPacket.setNetwork(network);
    dataPacket.setStation(station);
    dataPacket.setChannel(channel);
    dataPacket.setLocationCode(locationCode);
    dataPacket.setStartTime(startTime);
    EXPECT_NO_THROW(dataPacket.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(dataPacket.setData(timeSeries));

    DataPacket packetCopy(dataPacket);
    // Verify 
    EXPECT_EQ(packetCopy.getMessageType(), messageType);
    EXPECT_EQ(packetCopy.getStartTime(), startTimeMuS);
    EXPECT_NEAR(packetCopy.getSamplingRate(), samplingRate, tol);
    EXPECT_EQ(packetCopy.getNetwork(), network);
    EXPECT_EQ(packetCopy.getStation(), station);
    EXPECT_EQ(packetCopy.getChannel(), channel);
    EXPECT_EQ(packetCopy.getLocationCode(), locationCode);
    EXPECT_EQ(packetCopy.getNumberOfSamples(),
              static_cast<int> (timeSeries.size()));
    EXPECT_EQ(packetCopy.getEndTime(), endTimeMuS);
    auto traceBack = packetCopy.getData();
    EXPECT_EQ(traceBack.size(), timeSeries.size());
    for (int i = 0; i < static_cast<int> (traceBack.size()); ++i)
    {
        auto res = static_cast<double> (traceBack[i] - timeSeries[i]);
        EXPECT_NEAR(res, 0, tol);
    }

    auto traceMessage = packetCopy.toMessage();
    packetCopy.clear();
    EXPECT_EQ(packetCopy.getNumberOfSamples(), 0);
    EXPECT_NO_THROW(packetCopy.fromMessage(traceMessage));
    EXPECT_EQ(packetCopy.getMessageType(), messageType);
    EXPECT_EQ(packetCopy.getStartTime(), startTimeMuS);
    EXPECT_NEAR(packetCopy.getSamplingRate(), samplingRate, tol);
    EXPECT_EQ(packetCopy.getNetwork(), network);
    EXPECT_EQ(packetCopy.getStation(), station);
    EXPECT_EQ(packetCopy.getChannel(), channel);
    EXPECT_EQ(packetCopy.getLocationCode(), locationCode);
    EXPECT_EQ(packetCopy.getNumberOfSamples(),
              static_cast<int> (timeSeries.size()));
    EXPECT_EQ(packetCopy.getEndTime(), endTimeMuS);
    traceBack = packetCopy.getData();
    EXPECT_EQ(traceBack.size(), timeSeries.size());
    for (int i = 0; i < static_cast<int> (traceBack.size()); ++i)
    {
        auto res = static_cast<double> (traceBack[i] - timeSeries[i]);
        EXPECT_NEAR(res, 0, tol);
    }
}

}

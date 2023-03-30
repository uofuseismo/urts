#include <string>
#include <cmath>
#include <cstring>
#include <vector>
#include <chrono>
#include <limits>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/probabilityPacket/probabilityPacket.hpp"
#include "urts/broadcasts/internal/probabilityPacket/subscriberOptions.hpp"
#include "urts/broadcasts/internal/probabilityPacket/publisherOptions.hpp"
#include <gtest/gtest.h>
namespace
{

using namespace URTS::Broadcasts::Internal::ProbabilityPacket;

TEST(BroadcastsInternalProbabilityPacket, probabilityPacket)
{
    const std::string messageType{"URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket"};
    const std::string network{"UU"};
    const std::string station{"FORK"};
    const std::string channel{"HHP"};
    const std::string locationCode{"01"};
    const std::string algorithm{"UNet"};
    const std::vector<std::string> originalChannels{"HHZ", "HHN", "HHE"};
    const std::string positiveClassName{"P"};
    const std::string negativeClassName{"Noise"};
    const std::vector<double> timeSeries{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const double startTime{1628803598};
    const std::chrono::microseconds startTimeMuS{1628803598000000};
    const std::chrono::microseconds endTimeMuS{1628803598000000 + 225000}; // 225000 = std::round(9./40*1000000
    double samplingRate{40};
    auto tol = std::numeric_limits<double>::epsilon();

    ProbabilityPacket packet;
    packet.setNetwork(network);
    packet.setStation(station);
    packet.setChannel(channel);
    packet.setLocationCode(locationCode);
    packet.setStartTime(startTime);
    packet.setAlgorithm(algorithm);
    packet.setOriginalChannels(originalChannels);
    packet.setPositiveClassName(positiveClassName);
    packet.setNegativeClassName(negativeClassName);
    EXPECT_NO_THROW(packet.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(packet.setData(timeSeries));

    ProbabilityPacket packetCopy(packet);
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
    EXPECT_EQ(packetCopy.getAlgorithm(), algorithm);
    EXPECT_EQ(packetCopy.getPositiveClassName(), positiveClassName);
    EXPECT_EQ(packetCopy.getNegativeClassName(), negativeClassName);
    auto originalChannelsBack = packetCopy.getOriginalChannels();
    EXPECT_EQ(originalChannelsBack.size(), originalChannels.size());
    for (int i = 0; i < static_cast<int> (originalChannels.size()); ++i)
    {
        EXPECT_EQ(originalChannels[i], originalChannelsBack[i]);
    }
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

TEST(BroadcastsInternalProbabilityPacket, SubscriberOptions)
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int recvHWM{106};
    const std::chrono::milliseconds recvTimeOut{145};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    SubscriberOptions options;
    EXPECT_NO_THROW(options.setAddress(address));
    EXPECT_NO_THROW(options.setHighWaterMark(recvHWM));
    EXPECT_NO_THROW(options.setTimeOut(recvTimeOut));
    EXPECT_NO_THROW(options.setZAPOptions(zapOptions));

    SubscriberOptions copy(options);
    EXPECT_EQ(options.getAddress(), address);
    EXPECT_EQ(options.getHighWaterMark(), recvHWM);
    EXPECT_EQ(options.getTimeOut(), recvTimeOut);
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());    

    options.clear();
    EXPECT_FALSE(options.haveAddress());
    EXPECT_EQ(options.getHighWaterMark(), 8192);
    EXPECT_EQ(options.getTimeOut(), std::chrono::milliseconds {10});
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              UMPS::Authentication::SecurityLevel::Grasslands);
}

TEST(BroadcastsInternalProbabilityPacket, PublisherOptions)
{
    PublisherOptions options;
    const std::string address{"tcp://127.0.0.1:8080"};
    const int sendHWM{113};
    const std::chrono::milliseconds sendTimeOut{155};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    EXPECT_NO_THROW(options.setAddress(address));
    EXPECT_NO_THROW(options.setHighWaterMark(sendHWM));
    EXPECT_NO_THROW(options.setTimeOut(sendTimeOut));
    EXPECT_NO_THROW(options.setZAPOptions(zapOptions));

    PublisherOptions copy(options);
    EXPECT_EQ(options.getAddress(), address);
    EXPECT_EQ(options.getHighWaterMark(), sendHWM);
    EXPECT_EQ(options.getTimeOut(), sendTimeOut);
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());    

    options.clear();
    EXPECT_FALSE(options.haveAddress());
    EXPECT_EQ(options.getHighWaterMark(), 8192);
    EXPECT_EQ(options.getTimeOut(), std::chrono::milliseconds {1000});
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              UMPS::Authentication::SecurityLevel::Grasslands);
}

}

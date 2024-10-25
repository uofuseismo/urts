#include <string>
#include <cmath>
#include <cstring>
#include <vector>
#include <chrono>
#include <limits>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriberOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/publisherOptions.hpp"

using namespace URTS::Broadcasts::Internal::DataPacket;

TEST_CASE("URTS::Broadcasts::Internal::DataPacket", "[DataPacket]")
{
    const std::string messageType{"URTS::Broadcasts::Internal::DataPacket::DataPacket"};
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
    REQUIRE_NOTHROW(dataPacket.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(dataPacket.setData(timeSeries));

    DataPacket packetCopy(dataPacket);
    // Verify 
    CHECK(packetCopy.getMessageType() == messageType);
    CHECK(packetCopy.getStartTime() == startTimeMuS);
    CHECK(std::abs(packetCopy.getSamplingRate() - samplingRate) < tol);
    CHECK(packetCopy.getNetwork() == network);
    CHECK(packetCopy.getStation() == station);
    CHECK(packetCopy.getChannel() == channel);
    CHECK(packetCopy.getLocationCode() == locationCode);
    CHECK(packetCopy.getNumberOfSamples() ==
          static_cast<int> (timeSeries.size()));
    CHECK(packetCopy.getEndTime() == endTimeMuS);
    auto traceBack = packetCopy.getData();
    REQUIRE(traceBack.size() == timeSeries.size());
    for (int i = 0; i < static_cast<int> (traceBack.size()); ++i)
    {
        auto res = static_cast<double> (traceBack[i] - timeSeries[i]);
        CHECK(std::abs(res) < tol);
    }

    SECTION("From Message")
    {
    auto traceMessage = packetCopy.toMessage();
    packetCopy.clear();
    CHECK(packetCopy.getNumberOfSamples() == 0);
    REQUIRE_NOTHROW(packetCopy.fromMessage(traceMessage));
    CHECK(packetCopy.getMessageType() == messageType);
    CHECK(packetCopy.getStartTime() == startTimeMuS);
    CHECK(std::abs(packetCopy.getSamplingRate() - samplingRate) < tol);
    CHECK(packetCopy.getNetwork() == network);
    CHECK(packetCopy.getStation() == station);
    CHECK(packetCopy.getChannel() == channel);
    CHECK(packetCopy.getLocationCode() == locationCode);
    CHECK(packetCopy.getNumberOfSamples() ==
          static_cast<int> (timeSeries.size()));
    CHECK(packetCopy.getEndTime() == endTimeMuS);
    traceBack = packetCopy.getData();
    REQUIRE(traceBack.size() == timeSeries.size());
    for (int i = 0; i < static_cast<int> (traceBack.size()); ++i)
    {
        auto res = static_cast<double> (traceBack[i] - timeSeries[i]);
        CHECK(std::abs(res) < tol);
    }
    }
}

TEST_CASE("URTS::Broadcasts::Internal::DataPacket", "[SubscriberOptions]")
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int recvHWM{106};
    const std::chrono::milliseconds recvTimeOut{145};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    SubscriberOptions options;
    REQUIRE_NOTHROW(options.setAddress(address));
    REQUIRE_NOTHROW(options.setHighWaterMark(recvHWM));
    REQUIRE_NOTHROW(options.setTimeOut(recvTimeOut));
    REQUIRE_NOTHROW(options.setZAPOptions(zapOptions));

    SubscriberOptions copy(options);
    CHECK(options.getAddress() == address);
    CHECK(options.getHighWaterMark() == recvHWM);
    CHECK(options.getTimeOut() == recvTimeOut);
    CHECK(options.getZAPOptions().getSecurityLevel() ==
          zapOptions.getSecurityLevel());    

    SECTION("clear")
    {
    options.clear();
    REQUIRE_FALSE(options.haveAddress());
    CHECK(options.getHighWaterMark() == 8192);
    CHECK(options.getTimeOut() == std::chrono::milliseconds {10});
    CHECK(options.getZAPOptions().getSecurityLevel() ==
          UMPS::Authentication::SecurityLevel::Grasslands);
    }
}

TEST_CASE("URTS::Broadcasts::Internal::DataPacket", "[PublisherOptions]")
{
    PublisherOptions options;
    const std::string address{"tcp://127.0.0.1:8080"};
    const int sendHWM{113};
    const std::chrono::milliseconds sendTimeOut{155};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    REQUIRE_NOTHROW(options.setAddress(address));
    REQUIRE_NOTHROW(options.setHighWaterMark(sendHWM));
    REQUIRE_NOTHROW(options.setTimeOut(sendTimeOut));
    REQUIRE_NOTHROW(options.setZAPOptions(zapOptions));

    PublisherOptions copy(options);
    CHECK(options.getAddress() == address);
    CHECK(options.getHighWaterMark() == sendHWM);
    CHECK(options.getTimeOut() == sendTimeOut);
    CHECK(options.getZAPOptions().getSecurityLevel() ==
          zapOptions.getSecurityLevel());    

    options.clear();
    SECTION("clear")
    {
    REQUIRE_FALSE(options.haveAddress());
    CHECK(options.getHighWaterMark() == 8192);
    CHECK(options.getTimeOut() == std::chrono::milliseconds {1000});
    CHECK(options.getZAPOptions().getSecurityLevel() ==
          UMPS::Authentication::SecurityLevel::Grasslands);
    }
}

#include <chrono>
#include "urts/services/scalable/packetCache/bulkDataRequest.hpp"
#include "urts/services/scalable/packetCache/dataRequest.hpp"
#include "urts/proxyBroadcasts/dataPacket/dataPacket.hpp"
#include <gtest/gtest.h>

namespace
{

using namespace URTS::Services::Scalable::PacketCache;

TEST(ServicesScalablePacketCache, DataRequest)
{
    DataRequest request;
    const std::string network = "UU";
    const std::string station = "ARUT";
    const std::string channel = "EHZ";
    const std::string locationCode = "01";
    const uint64_t id = 400038;
    double t0 = 1629737861;
    double t1 = 1629737865;
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::PacketCache::DataRequest");
    EXPECT_NO_THROW(request.setNetwork(network));
    EXPECT_NO_THROW(request.setStation(station));
    EXPECT_NO_THROW(request.setChannel(channel));
    EXPECT_NO_THROW(request.setLocationCode(locationCode));
    request.setIdentifier(id);
    request.setQueryTime(t0); 
    auto [qTimeStart, qTimeEnd] = request.getQueryTimes();
    EXPECT_NEAR(qTimeStart, t0, 1.e-5);
    EXPECT_EQ(qTimeEnd, std::numeric_limits<uint32_t>::max());
    EXPECT_NO_THROW(request.setQueryTimes(std::pair(t0, t1)));

    auto message = request.toMessage();
    DataRequest requestCopy;
    EXPECT_NO_THROW(requestCopy.fromMessage(message));
    EXPECT_EQ(requestCopy.getNetwork(), network);
    EXPECT_EQ(requestCopy.getStation(), station);
    EXPECT_EQ(requestCopy.getChannel(), channel);
    EXPECT_EQ(requestCopy.getLocationCode(), locationCode);
    EXPECT_EQ(requestCopy.getIdentifier(), id);
    auto [timeStart, timeEnd] = requestCopy.getQueryTimes();
    EXPECT_NEAR(timeStart, t0, 1.e-5);
    EXPECT_NEAR(timeEnd,   t1, 1.e-5);
}

TEST(PacketCache, BulkDataRequest)
{
    BulkDataRequest bulkRequest;
    const std::string network = "UU";
    const std::string station = "ARUT";
    const std::vector<std::string> channels{"EHZ", "EHN", "EHE"};
    int nRequests = static_cast<int> (channels.size());
    const std::string locationCode = "01";
    const uint64_t id = 400038;
    double t0 = 1629737861;
    double t1 = 1629737865;
    EXPECT_EQ(bulkRequest.getMessageType(),
              "URTS::Services::Scalable::PacketCache::BulkDataRequest");
    DataRequest request;
    EXPECT_NO_THROW(request.setNetwork(network));
    EXPECT_NO_THROW(request.setStation(station));
    EXPECT_NO_THROW(request.setLocationCode(locationCode));
    request.setQueryTimes(std::pair {t0, t1});
    for (int i = 0; i < nRequests; ++i)
    {
        EXPECT_NO_THROW(request.setChannel(channels.at(i)));
        request.setIdentifier(id + i);
        bulkRequest.addDataRequest(request);
    }
    bulkRequest.setIdentifier(id);
    // Check for a duplicate - this is the most common case
    request.setIdentifier(id + 0);
    request.setChannel(channels.at(0));
    EXPECT_THROW(bulkRequest.addDataRequest(request), std::invalid_argument);

    EXPECT_EQ(bulkRequest.getNumberOfDataRequests(), nRequests);

    auto message = bulkRequest.toMessage();
    BulkDataRequest bulkRequestCopy;
    EXPECT_NO_THROW(bulkRequestCopy.fromMessage(message));

    EXPECT_EQ(bulkRequestCopy.getNumberOfDataRequests(), nRequests);
    EXPECT_EQ(bulkRequestCopy.getIdentifier(), id);
    auto requestsPtr = bulkRequestCopy.getDataRequestsPointer();
    for (int i = 0; i < nRequests; ++i)
    {
        EXPECT_EQ(requestsPtr[i].getNetwork(), network);
        EXPECT_EQ(requestsPtr[i].getStation(), station);
        EXPECT_EQ(requestsPtr[i].getChannel(), channels.at(i));
        EXPECT_EQ(requestsPtr[i].getLocationCode(), locationCode);
        EXPECT_EQ(requestsPtr[i].getIdentifier(), id + i);
        auto [timeStart, timeEnd] = requestsPtr[i].getQueryTimes();
        EXPECT_NEAR(timeStart, t0, 1.e-5);
        EXPECT_NEAR(timeEnd,   t1, 1.e-5);
    }
}

}

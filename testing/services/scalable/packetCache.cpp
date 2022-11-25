#include <fstream>
#include <cmath>
#include <limits>
#include "urts/services/scalable/packetCache/bulkDataRequest.hpp"
#include "urts/services/scalable/packetCache/bulkDataResponse.hpp"
#include "urts/services/scalable/packetCache/dataRequest.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/services/scalable/packetCache/sensorRequest.hpp"
#include "urts/services/scalable/packetCache/sensorResponse.hpp"
#include "urts/services/scalable/packetCache/wigginsInterpolator.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include <gtest/gtest.h>

namespace
{

using namespace URTS::Services::Scalable::PacketCache;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;

bool operator==(const UDP::DataPacket &lhs, const UDP::DataPacket &rhs)
{
    if (lhs.haveNetwork() == rhs.haveNetwork())
    {
        if (lhs.haveNetwork() && rhs.haveNetwork())
        {   
            if (lhs.getNetwork() != rhs.getNetwork()){return false;}
        }
    }
    else
    {
        return false;
    }

    if (lhs.haveStation() == rhs.haveStation())
    {
        if (lhs.haveStation() && rhs.haveStation())
        {
            if (lhs.getStation() != rhs.getStation()){return false;}
        }
    }
    else
    {
        return false;
    }

    if (lhs.haveChannel() == rhs.haveChannel())
    {
        if (lhs.haveChannel() && rhs.haveChannel())
        {
            if (lhs.getChannel() != rhs.getChannel()){return false;}
        }
    }
    else
    {
        return false;
    }

    if (lhs.haveLocationCode() == rhs.haveLocationCode())
    {
        if (lhs.haveLocationCode() && rhs.haveLocationCode())
        {
            if (lhs.getLocationCode() != rhs.getLocationCode()){return false;}
        }
    }
    else
    {
        return false;
    }

    if (lhs.haveSamplingRate() == rhs.haveSamplingRate())
    {
        if (lhs.haveSamplingRate() && rhs.haveSamplingRate())
        {
            if (std::abs(lhs.getSamplingRate()-rhs.getSamplingRate()) > 1.e-14)
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    if (lhs.getStartTime() != rhs.getStartTime()){return false;}
    if (lhs.getNumberOfSamples() != rhs.getNumberOfSamples()){return false;}
    if (lhs.getNumberOfSamples() > 0)
    {
        if (lhs.getEndTime() != rhs.getEndTime()){return false;}
        auto lv = lhs.getData();
        auto rv = rhs.getData();
        for (int i = 0; i < static_cast<int> (lv.size()); ++i)
        {
            if (std::abs(lv[i] - rv[i]) > std::numeric_limits<double>::epsilon())
            {
                return false;
            }
        }
    }
    return true;
}

TEST(ServicesScalablePacketCache, SensorRequest)
{
    SensorRequest request;
    const uint64_t id{600238};
    request.setIdentifier(id);
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::PacketCache::SensorRequest");
 
    auto message = request.toMessage();
    SensorRequest requestCopy;
    EXPECT_NO_THROW(requestCopy.fromMessage(message));
    EXPECT_EQ(requestCopy.getIdentifier(), id);
}

TEST(PacketCache, SensorResponse)
{
    const std::unordered_set<std::string> names{"UU.FORK.HHN.01",
                                                "UU.FORK.HHE.01",
                                                "UU.FORK.HHZ.01",
                                                "WY.YFT.EHZ.01"};
    SensorResponse response;
    auto rc = SensorResponse::ReturnCode::InvalidMessage;
    const uint64_t id{600238};
    response.setIdentifier(id);
    response.setReturnCode(rc);
    EXPECT_NO_THROW(response.setNames(names));
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Scalable::PacketCache::SensorResponse");

    auto message = response.toMessage();
    SensorResponse responseCopy;
    EXPECT_NO_THROW(responseCopy.fromMessage(message));
    EXPECT_EQ(responseCopy.getIdentifier(), id);
    EXPECT_EQ(responseCopy.getReturnCode(), rc);
    // There's really no guarantee how the names come back
    auto namesBack = responseCopy.getNames();
    EXPECT_EQ(namesBack.size(), names.size());
    for (const auto &n : namesBack)
    {
        EXPECT_TRUE(names.contains(n));
    }

    // Make sure I can deal with no names
    response.clear();
    message = response.toMessage();
    responseCopy.fromMessage(message.data(), message.size());
    EXPECT_EQ(response.getIdentifier(), 0);
    EXPECT_EQ(response.getReturnCode(), SensorResponse::ReturnCode::Success);
    namesBack = responseCopy.getNames();
    EXPECT_TRUE(namesBack.empty());
}

TEST(ServicesScalablePacketCache, DataRequest)
{
    DataRequest request;
    const std::string network{"UU"};
    const std::string station{"ARUT"};
    const std::string channel{"EHZ"};
    const std::string locationCode{"01"};
    const uint64_t id{400038};
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

TEST(ServicesScalablePacketCache, DataResponse)
{
    const std::string network{"UU"};
    const std::string station{"VRUT"};
    const std::string channel{"EHZ"};
    const std::string locationCode{"01"};
    const double samplingRate = 100;
    const uint64_t id = 594382;
    std::vector<UDP::DataPacket> dataPackets;
    const double t0 = 0;
    std::vector<double> startTimes;
    const std::vector<int> samplesPerPacket{100, 200, 100, 200};
    auto t1 = t0; 
    for (const auto &nSamples : samplesPerPacket)
    {   
        startTimes.push_back(t1);
        UDP::DataPacket dataPacket;
        dataPacket.setNetwork(network);
        dataPacket.setStation(station);
        dataPacket.setChannel(channel);
        dataPacket.setLocationCode(locationCode);
        dataPacket.setSamplingRate(samplingRate);
        dataPacket.setStartTime(t1);
        std::vector<double> data(nSamples);
        std::fill(data.begin(), data.end(), static_cast<double> (nSamples));
        dataPacket.setData(data);
        dataPackets.push_back(dataPacket);
        // Update start time
        t1 = t1 + std::round( (nSamples - 1)/samplingRate );
    }   
    DataResponse response;
    auto rc = DataResponse::ReturnCode::InvalidMessage;
    EXPECT_NO_THROW(response.setPackets(dataPackets));
    EXPECT_NO_THROW(response.setIdentifier(id));
    EXPECT_NO_THROW(response.setReturnCode(rc));

    // Reconstitute the class from a message 
    auto message = response.toMessage();
    DataResponse responseCopy;
    EXPECT_NO_THROW(responseCopy.fromMessage(message));
    EXPECT_EQ(responseCopy.getIdentifier(), id);
    EXPECT_EQ(responseCopy.getReturnCode(), rc);
    EXPECT_EQ(responseCopy.getMessageType(),
              "URTS::Services::Scalable::PacketCache::DataResponse");
    auto packetsBack = responseCopy.getPackets();
    EXPECT_EQ(packetsBack.size(), dataPackets.size());
    for (size_t i = 0; i < packetsBack.size(); ++i)
    {
        EXPECT_TRUE(packetsBack.at(i) == dataPackets.at(i));
    }

    // Set reversed packets
    std::reverse(packetsBack.begin(), packetsBack.end());
    response.setPackets(packetsBack);
    message = response.toMessage();
    EXPECT_NO_THROW(responseCopy.fromMessage(message));
    packetsBack = responseCopy.getPackets();
    for (size_t i = 0; i < packetsBack.size(); ++i)
    {
        EXPECT_TRUE(packetsBack.at(i) == dataPackets.at(i));
    }

    // See what happens when multiple packets start at same time.
    // This shouldn't result in a sort.
    dataPackets[0].setStartTime(0);
    dataPackets[1].setStartTime(0);
    EXPECT_NO_THROW(responseCopy.setPackets(dataPackets));
    packetsBack = responseCopy.getPackets();
    for (size_t i = 0; i < packetsBack.size(); ++i)
    {
        EXPECT_TRUE(packetsBack.at(i) == dataPackets.at(i));
    }
}

TEST(ServicesScalablePacketCache, BulkDataRequest)
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

TEST(ServicesScalablePacketCache, BulkDataResponse)
{
    const std::string network{"UU"};
    const std::string station{"VRUT"};
    const std::vector<std::string> channels{"EHZ", "EHN", "EHE"};
    const std::string locationCode{"01"};
    const double samplingRate{100};
    const uint64_t id{594382};
    std::vector<UDP::DataPacket> zDataPackets, nDataPackets, eDataPackets;
    const double t0 = 0;
    std::vector<double> startTimes;
    const std::vector<int> samplesPerPacket{100, 200, 100, 200};
    auto t1 = t0; 
    for (const auto &nSamples : samplesPerPacket)
    {   
        startTimes.push_back(t1);
        UDP::DataPacket dataPacket;
        dataPacket.setNetwork(network);
        dataPacket.setStation(station);
        dataPacket.setChannel(channels.at(0));
        dataPacket.setLocationCode(locationCode);
        dataPacket.setSamplingRate(samplingRate);
        dataPacket.setStartTime(t1);
        std::vector<double> data(nSamples); 
        std::fill(data.begin(), data.end(), static_cast<double> (nSamples));
        dataPacket.setData(data);
        zDataPackets.push_back(dataPacket);
 
        dataPacket.setChannel(channels.at(1));
        nDataPackets.push_back(dataPacket);

        dataPacket.setChannel(channels.at(2));
        eDataPackets.push_back(dataPacket);
        // Update start time
        t1 = t1 + std::round( (nSamples - 1)/samplingRate );
    }   
    BulkDataResponse bulkResponse;
    DataResponse response;
    auto dataRC = DataResponse::ReturnCode::InvalidMessage;
    response.setPackets(zDataPackets);
    response.setIdentifier(id + 1);
    response.setReturnCode(dataRC);
    EXPECT_NO_THROW(bulkResponse.addDataResponse(response));

    response.setPackets(nDataPackets);
    response.setIdentifier(id + 2);
    response.setReturnCode(dataRC);
    EXPECT_NO_THROW(bulkResponse.addDataResponse(response));

    response.setPackets(eDataPackets);
    response.setIdentifier(id + 3);
    response.setReturnCode(dataRC);
    EXPECT_NO_THROW(bulkResponse.addDataResponse(response));

    EXPECT_EQ(bulkResponse.getNumberOfDataResponses(), 3);
    bulkResponse.setIdentifier(id);
    bulkResponse.setReturnCode(BulkDataResponse::ReturnCode::NoSensor);

    // Reconsitute the bulk response
    auto message = bulkResponse.toMessage();
    BulkDataResponse brCopy;
    EXPECT_NO_THROW(brCopy.fromMessage(message));

    EXPECT_EQ(brCopy.getMessageType(),
              "URTS::Services::Scalable::PacketCache::BulkDataResponse");
    EXPECT_EQ(brCopy.getReturnCode(), BulkDataResponse::ReturnCode::NoSensor);
    EXPECT_EQ(brCopy.getNumberOfDataResponses(), 3);
    EXPECT_EQ(brCopy.getIdentifier(), id);
    auto responses = brCopy.getDataResponses();
    int i = 0;
    for (const auto &r : responses)
    {
        EXPECT_EQ(r.getReturnCode(), DataResponse::ReturnCode::InvalidMessage);
        EXPECT_EQ(r.getIdentifier(), id + i + 1);
        auto packetsBack = r.getPackets();
        EXPECT_EQ(packetsBack.size(), samplesPerPacket.size());
        if (i == 0)
        {
            for (int j = 0; j < static_cast<int> (packetsBack.size()); ++j)
            {
                EXPECT_TRUE(packetsBack.at(j) == zDataPackets.at(j));
            }
        }
        else if (i == 1)
        {
            for (int j = 0; j < static_cast<int> (packetsBack.size()); ++j)
            {
                EXPECT_TRUE(packetsBack.at(j) == nDataPackets.at(j));
            }
        }
        else if (i == 2)
        {
            for (int j = 0; j < static_cast<int> (packetsBack.size()); ++j)
            {
                EXPECT_TRUE(packetsBack.at(j) == eDataPackets.at(j));
            }
        }
        else
        {
            ASSERT_TRUE(false);
        }
        i = i + 1;
    }
}

TEST(ServicesScalablePacketCache, Wiggins)
{
    const double samplingRate{200};
    const double targetSamplingRate{250};
    const double targetSamplingPeriod = 1/targetSamplingRate;
    std::chrono::microseconds gapTolerance{55000};
    auto infl = std::ifstream("data/gse2.txt");
    double yDiff{0};
    std::vector<double> t;
    std::vector<double> x;
    t.reserve(12000);
    x.reserve(12000);
    std::string line;
    while (std::getline(infl, line))
    {
        t.push_back(t.size()/samplingRate);
        x.push_back(std::stod(line));
    }   
    infl.close();
    EXPECT_EQ(x.size(), 12000);
    std::vector<double> newTimes;
    std::vector<double> yRef;
    newTimes.reserve(14999);
    yRef.reserve(14999);
    infl = std::ifstream("data/wigint.txt");
    while (std::getline(infl, line))
    {   
        double t, yi; 
        sscanf(line.c_str(), "%lf,%lf\n", &t, &yi);
        newTimes.push_back(t);
        yRef.push_back(yi);
    }   
    infl.close();
    EXPECT_EQ(yRef.size(), 14999);
    // Interpolate this
    /*
    auto yi = weightedAverageSlopes(t, x, newTimes.front(),
                                    newTimes.back(), targetSamplingRate);
    EXPECT_EQ(yi.size(), yRef.size());
    double yDiff = 0;
    for (int i = 0; i < static_cast<int> (yi.size()); ++i)
    {   
        yDiff = std::max(yDiff, std::abs(yi.at(i) - yRef.at(i)));
    }
    EXPECT_NEAR(yDiff, 0, 1.e-8);
    */
    // Packetize this data
    auto n = static_cast<int> (x.size());
    std::vector<UDP::DataPacket> packets;
    int packetSize = 100;
    int i0 = 0;
    double t0 = 1644516968;
    std::chrono::microseconds t0MuSec{static_cast<int64_t> (std::round(t0*1000000))};
    double t1 = t0 + (yRef.size() - 1)*targetSamplingPeriod;
    std::chrono::microseconds t1MuSec{static_cast<int64_t> (std::round(t1*1000000))};
    for (int i = 0; i < n; ++i)
    {
        UDP::DataPacket packet;
        auto i1 = std::min(i0 + packetSize, n);
        auto nCopy = i1 - i0;
        packet.setNetwork("UU");
        packet.setStation("GH2");
        packet.setChannel("EHZ");
        packet.setLocationCode("01");
        //auto startTime = static_cast<int64_t>
        //                 ( std::round( (t0 + i0*(1./samplingRate))*1.e6 ) );
        //packet.setStartTime(std::chrono::microseconds{startTime} );
        packet.setStartTime(t0 + i0/samplingRate);
        packet.setSamplingRate(samplingRate);
        packet.setData(nCopy, &x[i0]);
        packets.push_back(std::move(packet));
        i0 = i1;
        if (i0 == n){break;}
    }
    auto packetsCopy = packets;
    // Interpolate it
    WigginsInterpolator interpolator;
    EXPECT_NO_THROW(interpolator.setTargetSamplingRate(targetSamplingRate));
    EXPECT_NO_THROW(interpolator.setGapTolerance(gapTolerance));
    EXPECT_NO_THROW(interpolator.interpolate(packets));
    EXPECT_EQ(interpolator.getStartTime(), t0MuSec);
    EXPECT_EQ(interpolator.getEndTime(), t1MuSec);
    EXPECT_EQ(interpolator.getNumberOfSamples(),
              static_cast<int> (newTimes.size()));
    auto yi = interpolator.getSignal();
    yDiff = 0;
    for (int i = 0; i < static_cast<int> (yi.size()); ++i)
    {
        yDiff = std::max(yDiff, std::abs(yi.at(i) - yRef.at(i)));
    }
    EXPECT_NEAR(yDiff, 0, 1.e-8);
    // There should be no gaps
    auto gapPtr = interpolator.getGapIndicatorPointer();
    for (int i = 0; i < interpolator.getNumberOfSamples(); ++i)
    {
        EXPECT_EQ(gapPtr[i], 0);
    }
    // Try clearing the signal
    interpolator.clearSignal();
    EXPECT_NEAR(interpolator.getTargetSamplingRate(),
                targetSamplingRate, 1.e-14);
    EXPECT_EQ(interpolator.getNumberOfSamples(), 0);
    EXPECT_EQ(interpolator.getGapTolerance(), gapTolerance);
    // Add some duplicate packets (really wherever)
    packets.push_back(packets[0]);
    packets.push_back(packets[1]);
    // And change the order
    std::srand(500582);
    std::random_shuffle(packets.begin(), packets.end());
    EXPECT_NO_THROW(interpolator.interpolate(packets));
    EXPECT_EQ(interpolator.getStartTime(), t0MuSec);
    EXPECT_EQ(interpolator.getEndTime(), t1MuSec);
    EXPECT_EQ(interpolator.getNumberOfSamples(),
              static_cast<int> (newTimes.size()));
    yi = interpolator.getSignal();
    yDiff = 0;
    for (int i = 0; i < static_cast<int> (yi.size()); ++i)
    {
        yDiff = std::max(yDiff, std::abs(yi.at(i) - yRef.at(i)));
    }
    EXPECT_NEAR(yDiff, 0, 1.e-8);
    // Make a gap and let the interpolator chew
    packets = packetsCopy;
    auto packet1 = packets.at(1);
    auto packet5 = packets.at(5);
    packets.erase(packets.begin() + 5);
    packets.erase(packets.begin() + 1);
    interpolator.clearSignal();
    EXPECT_NO_THROW(interpolator.interpolate(packets));
    gapPtr = interpolator.getGapIndicatorPointer();
    yi = interpolator.getSignal();
    EXPECT_EQ(yi.size(), yRef.size());
    yDiff = 0;
    // Note, the interpolation happens at previous packet's end time
    // and subsequent packet's start time
    auto t0Packet1 = packet1.getStartTime().count() - static_cast<int64_t> (targetSamplingPeriod*1.e6);
    auto t1Packet1 = packet1.getEndTime().count()   + static_cast<int64_t> (targetSamplingPeriod*1.e6); 
    auto t0Packet5 = packet5.getStartTime().count() - static_cast<int64_t> (targetSamplingPeriod*1.e6);
    auto t1Packet5 = packet5.getEndTime().count()   + static_cast<int64_t> (targetSamplingPeriod*1.e6);
    for (int i = 0; i < static_cast<int> (yi.size()); ++i)
    {
        auto time = t0MuSec.count()
                  + static_cast<int64_t>
                    (std::round(i*targetSamplingPeriod*1.e6));
        if ( (time > t0Packet1 && time < t1Packet1) ||
             (time > t0Packet5 && time < t1Packet5) )
        {
            EXPECT_EQ(gapPtr[i], 1);
        }
        else
        {
            EXPECT_EQ(gapPtr[i], 0);
        }
    }
    // Shuffle and repeat
    std::random_shuffle(packets.begin(), packets.end());
    EXPECT_NO_THROW(interpolator.interpolate(packets));
    yi = interpolator.getSignal();
    gapPtr = interpolator.getGapIndicatorPointer();
    for (int i = 0; i < static_cast<int> (yi.size()); ++i)
    {
        auto time = t0MuSec.count()
                  + static_cast<int64_t>
                    (std::round(i*targetSamplingPeriod*1.e6));
        if ( (time > t0Packet1 && time < t1Packet1) ||
             (time > t0Packet5 && time < t1Packet5) )
        {
            EXPECT_EQ(gapPtr[i], 1);
        }
        else
        {
            EXPECT_EQ(gapPtr[i], 0);
        }
    }
 
}

}

#include <string>
#include <vector>
#include "urts/broadcasts/external/seedlink/clientOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include <gtest/gtest.h>
namespace
{

using namespace URTS::Broadcasts::External::SEEDLink;

TEST(BroadcastsExternalSEEDLink, ClientOptions)
{
    const std::string address{"127.0.0.1"};
    const uint16_t port{1540};
    const std::string stateFile{"seedlink.state"};
    const std::string defaultAddress{"rtserve.iris.washington.edu"};
    const std::chrono::seconds networkTimeOut{3838}; 
    const std::chrono::seconds reconnectDelay{38};
    const uint16_t defaultPort{18000};
    const uint16_t updateInterval{125};
    const int recordSize{256};
    const int maxQueueSize{9494};
    ClientOptions options;
    EXPECT_NO_THROW(options.setAddress(address));
    options.setPort(port);
    EXPECT_NO_THROW(options.setStateFile(stateFile));
    options.setStateFileUpdateInterval(updateInterval);
    EXPECT_NO_THROW(options.setSEEDRecordSize(recordSize));
    EXPECT_NO_THROW(options.setMaximumInternalQueueSize(maxQueueSize));
    EXPECT_NO_THROW(options.setNetworkTimeOut(networkTimeOut));
    EXPECT_NO_THROW(options.setNetworkReconnectDelay(reconnectDelay));

    ClientOptions copy(options);
    EXPECT_EQ(copy.getAddress(), address);
    EXPECT_EQ(copy.getPort(), port);
    EXPECT_EQ(copy.getStateFile(), stateFile);
    EXPECT_EQ(copy.getStateFileUpdateInterval(), updateInterval); 
    EXPECT_EQ(copy.getSEEDRecordSize(), recordSize);
    EXPECT_EQ(copy.getMaximumInternalQueueSize(), maxQueueSize);
    EXPECT_EQ(copy.getNetworkTimeOut(), networkTimeOut);
    EXPECT_EQ(copy.getNetworkReconnectDelay(), reconnectDelay);

    options.clear();
    EXPECT_EQ(options.getAddress(), defaultAddress);
    EXPECT_EQ(options.getPort(), defaultPort);
    EXPECT_FALSE(options.haveStateFile());
    EXPECT_EQ(options.getStateFileUpdateInterval(), 100);
    EXPECT_EQ(options.getSEEDRecordSize(), 512);
    EXPECT_EQ(options.getMaximumInternalQueueSize(), 8192);
    EXPECT_EQ(options.getNetworkTimeOut(), std::chrono::seconds {600});
    EXPECT_EQ(options.getNetworkReconnectDelay(), std::chrono::seconds {30});
}

}

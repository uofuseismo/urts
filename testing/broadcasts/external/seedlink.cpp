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

    ClientOptions copy(options);
    EXPECT_EQ(copy.getAddress(), address);
    EXPECT_EQ(copy.getPort(), port);
    EXPECT_EQ(copy.getStateFile(), stateFile);
    EXPECT_EQ(copy.getStateFileUpdateInterval(), updateInterval); 
    EXPECT_EQ(copy.getSEEDRecordSize(), recordSize);
    EXPECT_EQ(copy.getMaximumInternalQueueSize(), maxQueueSize);

    options.clear();
    EXPECT_EQ(options.getAddress(), defaultAddress);
    EXPECT_EQ(options.getPort(), defaultPort);
    EXPECT_FALSE(options.haveStateFile());
    EXPECT_EQ(options.getStateFileUpdateInterval(), 100);
    EXPECT_EQ(options.getSEEDRecordSize(), 512);
    EXPECT_EQ(options.getMaximumInternalQueueSize(), 8192);
}

}

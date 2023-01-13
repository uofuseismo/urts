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
    const std::string defaultAddress{"rtserve.iris.washington.edu"};
    const uint16_t defaultPort{18000};
    ClientOptions options;
    EXPECT_NO_THROW(options.setAddress(address));
    options.setPort(port);

    ClientOptions copy(options);
    EXPECT_EQ(copy.getAddress(), address);
    EXPECT_EQ(copy.getPort(), port);

    options.clear();
    EXPECT_EQ(options.getAddress(), defaultAddress);
    EXPECT_EQ(options.getPort(), defaultPort);
}

}

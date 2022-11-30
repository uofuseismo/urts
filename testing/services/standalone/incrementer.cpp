#include <fstream>
#include <thread>
#include <iostream>
#include <filesystem>
#include <umps/messaging/routerDealer/proxy.hpp>
#include <umps/messaging/routerDealer/proxyOptions.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/standalone/incrementer/counter.hpp"
#include "urts/services/standalone/incrementer/incrementRequest.hpp"
#include "urts/services/standalone/incrementer/incrementResponse.hpp"
#include "urts/services/standalone/incrementer/itemsRequest.hpp"
#include "urts/services/standalone/incrementer/itemsResponse.hpp"
#include "urts/services/standalone/incrementer/requestorOptions.hpp"
#include "urts/services/standalone/incrementer/requestor.hpp"
#include "urts/services/standalone/incrementer/serviceOptions.hpp"
#include "urts/services/standalone/incrementer/service.hpp"
#include <gtest/gtest.h>

namespace
{

using namespace URTS::Services::Standalone::Incrementer;

TEST(ServicesStandaloneIncrementer, Counter)
{
    Counter counter;
    const std::string tableName{"tables/counter.sqlite3"};
    const bool deleteIfExists{true};
    const std::string item{"test"};
    std::map<std::string, std::pair<int64_t, int32_t>> items;

    items.insert( std::pair("pick", std::pair(5, 10)) );
    items.insert( std::pair("event", std::pair(0, 1)) );


    EXPECT_NO_THROW(counter.initialize(tableName, items, deleteIfExists));
    EXPECT_TRUE(counter.isInitialized());
    EXPECT_TRUE(counter.haveItem("pick"));
    EXPECT_TRUE(counter.haveItem("event"));

    EXPECT_NO_THROW(counter.addItem("arrival", 4, 11));
    EXPECT_TRUE(counter.haveItem("arrival"));

    auto itemsInTable = counter.getItems();
    EXPECT_EQ(itemsInTable.size(), 3); 
    EXPECT_TRUE(itemsInTable.contains("pick")); 
    EXPECT_TRUE(itemsInTable.contains("arrival"));
    EXPECT_TRUE(itemsInTable.contains("event"));

    auto nextValue = counter.getNextValue("pick");
    EXPECT_EQ(nextValue, 5 + 10);
    nextValue = counter.getNextValue("pick");
    EXPECT_EQ(nextValue, 5 + 10 + 10);

    EXPECT_EQ(counter.getCurrentValue("pick"), 5 + 10 + 10);
    EXPECT_EQ(counter.getCurrentValue("arrival"), 4); 
    EXPECT_EQ(counter.getCurrentValue("event"), 0); 
}

TEST(ServicesStandaloneIncrementer, IncrementRequest)
{
    IncrementRequest request;
    uint64_t id{553};
    const std::string item{"abc"};

    EXPECT_EQ(request.getItemSet().size(), 6);
    EXPECT_NO_THROW(request.setItem(IncrementRequest::Item::PhasePick));
    EXPECT_EQ(request.getItem(), "PhasePick");
    EXPECT_NO_THROW(request.setItem(IncrementRequest::Item::PhaseArrival));
    EXPECT_EQ(request.getItem(), "PhaseArrival");
    EXPECT_NO_THROW(request.setItem(IncrementRequest::Item::Event));
    EXPECT_EQ(request.getItem(), "Event");
    EXPECT_NO_THROW(request.setItem(IncrementRequest::Item::Origin));
    EXPECT_EQ(request.getItem(), "Origin");
    EXPECT_NO_THROW(request.setItem(IncrementRequest::Item::Amplitude));
    EXPECT_EQ(request.getItem(), "Amplitude");
    EXPECT_NO_THROW(request.setItem(IncrementRequest::Item::Magnitude));
    EXPECT_EQ(request.getItem(), "Magnitude");

    EXPECT_NO_THROW(request.setItem(item));
    request.setIdentifier(id);
    EXPECT_EQ(request.getIdentifier(), id);

    auto msg = request.toMessage();
    IncrementRequest rCopy;
    EXPECT_NO_THROW(rCopy.fromMessage(msg));
    EXPECT_EQ(rCopy.getItem(), item);
    EXPECT_EQ(rCopy.getIdentifier(), id);

    request.clear();
    EXPECT_FALSE(request.haveItem());
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Standalone::Incrementer::IncrementRequest");
}


TEST(ServicesStandaloneIncrementer, IncrementResponse)
{
    IncrementResponse response;
    const uint64_t id{553};
    const int64_t value{3938};
    auto code = IncrementResponse::ReturnCode::NoItem;
    EXPECT_FALSE(response.haveValue());
    response.setValue(value);
    response.setIdentifier(id);
    EXPECT_TRUE(response.haveValue());
    EXPECT_EQ(response.getReturnCode(), IncrementResponse::ReturnCode::Success);
    response.setReturnCode(code);

    auto msg = response.toMessage();

    IncrementResponse rCopy;
    EXPECT_NO_THROW(rCopy.fromMessage(msg));
    EXPECT_EQ(rCopy.getValue(), value);
    EXPECT_EQ(rCopy.getIdentifier(), id);
    EXPECT_EQ(rCopy.getReturnCode(), code);

    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Standalone::Incrementer::IncrementResponse");
}

TEST(ServicesStandaloneIncrementer, ItemsRequest)
{
    ItemsRequest request;
    const uint64_t id{253};

    request.setIdentifier(id);
    EXPECT_EQ(request.getIdentifier(), id);

    auto msg = request.toMessage();
    ItemsRequest rCopy;
    EXPECT_NO_THROW(rCopy.fromMessage(msg));
    EXPECT_EQ(rCopy.getIdentifier(), id);

    request.clear();
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Standalone::Incrementer::ItemsRequest");
}

TEST(ServicesStandaloneIncrementer, ItemsResponse)
{
    ItemsResponse response;
    const uint64_t id{3423};
    const std::set<std::string> items{"abc", "123", "mj"};
    const auto returnCode = ItemsResponse::ReturnCode::InvalidMessage;
    
    response.setIdentifier(id);
    EXPECT_NO_THROW(response.setItems(items));
    response.setReturnCode(returnCode);
    
    auto msg = response.toMessage();
    ItemsResponse rCopy;
    EXPECT_NO_THROW(rCopy.fromMessage(msg));
    EXPECT_EQ(rCopy.getItems().size(), items.size());
    EXPECT_EQ(rCopy.getItems(), items);
    EXPECT_EQ(rCopy.getIdentifier(), id);
    EXPECT_EQ(response.getReturnCode(), returnCode);
    
    response.clear();
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Standalone::Incrementer::ItemsResponse");
}

TEST(ServicesStandaloneIncrementer, RequestorOptions)
{
    const std::string address{"ipc://packetCache.ipc"};
    const int sendHWM{102};
    const int recvHWM{103};
    const std::chrono::milliseconds sendTimeOut{100};
    const std::chrono::milliseconds recvTimeOut{105};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    RequestorOptions options;
    EXPECT_NO_THROW(options.setAddress(address));
    EXPECT_NO_THROW(options.setSendHighWaterMark(sendHWM));
    EXPECT_NO_THROW(options.setReceiveHighWaterMark(recvHWM));
    options.setSendTimeOut(sendTimeOut);
    options.setReceiveTimeOut(recvTimeOut);
    options.setZAPOptions(zapOptions);

    RequestorOptions copy(options);
    EXPECT_EQ(copy.getAddress(), address);
    EXPECT_EQ(copy.getSendHighWaterMark(), sendHWM);
    EXPECT_EQ(copy.getReceiveHighWaterMark(), recvHWM);
    EXPECT_EQ(copy.getSendTimeOut(), sendTimeOut); 
    EXPECT_EQ(copy.getReceiveTimeOut(), recvTimeOut);
    EXPECT_EQ(copy.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());

    options.clear();
    EXPECT_EQ(options.getSendHighWaterMark(), 4096);
    EXPECT_EQ(options.getReceiveHighWaterMark(), 4096);
    EXPECT_EQ(options.getSendTimeOut(), std::chrono::milliseconds {0});
    EXPECT_EQ(options.getReceiveTimeOut(), std::chrono::milliseconds {2000});
    zapOptions.setGrasslandsClient();
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());
}

TEST(ServicesStandaloneIncrementer, ServiceOptions)
{
    const char *cParms = "[Incrementer]\nsqlite3FileName = tables/incrementer.sqlite3\ninitialValue = 2\nincrement = 3\naddress = tcp://localhost:5560\nsendHighWaterMark = 40\nreceiveHighWaterMark = 50\npollingTimeOut = 12";
    //std::cout << cParms << std::endl;
    const std::string iniFileName = "incrementerExample.ini";
    std::ofstream tempFile(iniFileName);
    tempFile << cParms;
    tempFile.close();

    ServiceOptions options; 
    EXPECT_NO_THROW(options.parseInitializationFile(iniFileName));
    EXPECT_EQ(options.getSqlite3FileName(), "tables/incrementer.sqlite3");
    EXPECT_EQ(options.getInitialValue(), 2);
    EXPECT_EQ(options.getIncrement(), 3);
    EXPECT_EQ(options.getAddress(), "tcp://localhost:5560");
    EXPECT_EQ(options.getSendHighWaterMark(), 40);
    EXPECT_EQ(options.getReceiveHighWaterMark(), 50);
    EXPECT_EQ(options.getPollingTimeOut(), std::chrono::milliseconds {12});

    options.clear();
    EXPECT_EQ(options.getInitialValue(), 0);
    EXPECT_EQ(options.getIncrement(), 1);
    EXPECT_EQ(options.getSendHighWaterMark(), 1024);
    EXPECT_EQ(options.getReceiveHighWaterMark(), 1024);
    EXPECT_EQ(options.getPollingTimeOut(), std::chrono::milliseconds {10});
    EXPECT_FALSE(options.haveAddress());

    if (std::filesystem::exists(iniFileName))
    {
        std::filesystem::remove(iniFileName);
    }
}

///--------------------------------------------------------------------------///
///                               Communication Test                         ///
///--------------------------------------------------------------------------///
const std::string frontendAddress{"tcp://127.0.0.1:5555"};
const std::string backendAddress{"tcp://127.0.0.1:5556"};
const std::string sqlite3File{"test_counter.sqlite3"};
constexpr int initialValue{5};
constexpr int increment{5};

void proxy()
{
    UMPS::Messaging::RouterDealer::ProxyOptions options;
    options.setFrontendAddress(frontendAddress);
    options.setBackendAddress(backendAddress);
    UMPS::Messaging::RouterDealer::Proxy proxy;
    EXPECT_NO_THROW(proxy.initialize(options));
    std::thread proxyThread(&UMPS::Messaging::RouterDealer::Proxy::start,
                            &proxy);
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_TRUE(proxy.isRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_NO_THROW(proxy.stop());
    proxyThread.join();
}

void server()
{
    if (std::filesystem::exists(sqlite3File))
    {
        std::filesystem::remove(sqlite3File);
    }
    ServiceOptions options;
    options.setAddress(backendAddress);
    options.setSqlite3FileName(sqlite3File);
    options.setInitialValue(initialValue);
    options.setIncrement(increment);
    Service service;
    EXPECT_NO_THROW(service.initialize(options));
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_TRUE(service.isInitialized()); 
    EXPECT_NO_THROW(service.start());
    std::this_thread::sleep_for(std::chrono::milliseconds{1100});
    service.stop();

    std::filesystem::remove(sqlite3File);
}

void client()
{
    RequestorOptions options;
    options.setAddress(frontendAddress);
    Requestor client;
    EXPECT_NO_THROW(client.initialize(options));
    EXPECT_TRUE(client.isInitialized());
    ItemsRequest itemsRequest;
    itemsRequest.setIdentifier(1);
    // Make sure all the items are there
    auto referenceItems = IncrementRequest::getItemSet(); 
    auto itemsReply = client.request(itemsRequest);
    if (itemsReply != nullptr)
    {
        EXPECT_EQ(itemsReply->getReturnCode(),
                  ItemsResponse::ReturnCode::Success);
        auto names = itemsReply->getItems();
        for (const auto &name : referenceItems)
        {
            EXPECT_TRUE(names.contains(IncrementRequest::itemToString(name)));
        }
    }
    else
    {
        EXPECT_TRUE(false);
    }
    // Increment an amplitude and event
    IncrementRequest incrementRequest;
    incrementRequest.setItem("Amplitude");
    auto incrementReply = client.request(incrementRequest);
    EXPECT_TRUE(incrementReply->getValue() >= 10 ||
                incrementReply->getValue() <= 15);
    incrementRequest.setItem("Event");
    EXPECT_TRUE(incrementReply->getValue() >= 10 ||
                incrementReply->getValue() <= 15);
}


TEST(ServicesStandaloneIncrementer, Service)
{
    // Start the proxy
    auto proxyThread = std::thread(proxy);
    std::this_thread::sleep_for(std::chrono::milliseconds {50});
    // Connect / initialize the server
    auto serverThread = std::thread(server);
    // Let the clients rip
    std::this_thread::sleep_for(std::chrono::milliseconds {100});
    auto clientThread1 = std::thread(client);
    auto clientThread2 = std::thread(client);

    clientThread1.join();
    clientThread2.join();
    serverThread.join();
    proxyThread.join();
}

}

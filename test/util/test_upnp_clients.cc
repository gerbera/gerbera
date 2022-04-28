#include <gtest/gtest.h>
#include <upnp/upnp.h>

#include "config/client_config.h"
#include "util/grb_net.h"
#include "util/upnp_clients.h"

#include "../mock/config_mock.h"

class MyConfigMock final : public ConfigMock {
public:
    MyConfigMock() { list = std::make_shared<ClientConfigList>(); }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) const override { return list; }
    std::shared_ptr<ClientConfigList> list;
};

class UpnpClientsTest : public ::testing::Test {
public:
    UpnpClientsTest() = default;
    ~UpnpClientsTest() override = default;

    void SetUp() override
    {
        config = std::make_shared<MyConfigMock>();

        auto clientConfig = std::make_shared<ClientConfig>(123, "default", "192.168.1.100", "added by config", 1);
        config->list->add(clientConfig, 0);

        subject = new ClientManager(config, nullptr);
    }

    void TearDown() override
    {
        delete subject;
    }

    ClientManager* subject;
    std::shared_ptr<MyConfigMock> config;
};

TEST_F(UpnpClientsTest, bubbleUPnPV3_4_4)
{
    const ClientInfo* pInfo;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pInfo = subject->getInfo(addr, "Android/8.0.0 UPnP/1.0 BubbleUPnP/3.4.4");
    EXPECT_EQ(pInfo->type, ClientType::BubbleUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pInfo = subject->getInfo(addr, "BubbleUPnP UPnP/1.1");
    EXPECT_EQ(pInfo->type, ClientType::BubbleUPnP);
}

TEST_F(UpnpClientsTest, foobar2000V1_6_2)
{
    const ClientInfo* pInfo;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pInfo = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pInfo = subject->getInfo(addr, "foobar2000/1.6.2");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, kodiV18_9)
{
    const ClientInfo* pInfo;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pInfo = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Kodi");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pInfo = subject->getInfo(addr, "Kodi/18.9 (Windows NT 10.0.19041; Win64; x64) App_Bitness/64 Version/18.9-(18.9.0)-Git:20201023-0655c2c718");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, samsungTVQ70)
{
    const ClientInfo* pInfo;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pInfo = subject->getInfo(addr, "DLNADOC/1.50 SEC_HHP_[TV] Samsung Q70 Series/1.0 UPnP/1.0");
    EXPECT_EQ(pInfo->type, ClientType::SamsungSeriesQ);

    // 2. via fileInfo (e.g. info/open/read video)
    pInfo = subject->getInfo(addr, "samsung-agent/1.1");
    EXPECT_EQ(pInfo->type, ClientType::SamsungSeriesQ);
}

TEST_F(UpnpClientsTest, vlcV3_0_11_1)
{
    const ClientInfo* pInfo;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pInfo = subject->getInfo(addr, "Linux/5.9.0-5-amd64, UPnP/1.0, Portable SDK for UPnP devices/1.8.4");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pInfo = subject->getInfo(addr, "VLC/3.0.11.1 LibVLC/3.0.11.1");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, windows10)
{
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // via actionReq (e.g. doBrowse)
    const ClientInfo* pInfo = subject->getInfo(addr, "Microsoft-Windows/10.0 UPnP/1.0 Microsoft-DLNA DLNADOC/1.50");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, multipleClientsOnSameIP)
{
    const ClientInfo* pInfo;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. Foobar2000 via actionReq (e.g. doBrowse)
    pInfo = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. Kodi via actionReq (e.g. doBrowse)
    pInfo = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Kodi");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 3. Foobar2000 via fileInfo (e.g. info/open/read video)
    pInfo = subject->getInfo(addr, "foobar2000/1.6.2");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 4. Kodi via fileInfo (e.g. info/open/read video)
    pInfo = subject->getInfo(addr, "Kodi/18.9 (Windows NT 10.0.19041; Win64; x64) App_Bitness/64 Version/18.9-(18.9.0)-Git:20201023-0655c2c718");
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

// keep this at the end of all tests (otherwise we need a removeClientInfo function...)
TEST_F(UpnpClientsTest, configuredIP)
{
    auto addr = std::make_shared<GrbNet>("192.168.1.100");

    // act
    const ClientInfo* pInfo = subject->getInfo(addr, "any unknown user-agent info");
    EXPECT_EQ(pInfo->type, ClientType::Unknown);
    EXPECT_EQ(pInfo->flags, 123);
}

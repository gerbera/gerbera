/*GRB*
    Gerbera - https://gerbera.io/

    test_upnp_clients.cc - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

#include "config/result/client_config.h"
#include "util/grb_net.h"
#include "upnp/client_manager.h"
#include "upnp/clients.h"

#include "../mock/config_mock.h"

#include <gtest/gtest.h>
#include <upnp.h>

class MyConfigMock final : public ConfigMock {
public:
    MyConfigMock() { list = std::make_shared<ClientConfigList>(); }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(ConfigVal option) const override { return list; }
    std::shared_ptr<ClientConfigList> list;
};

class UpnpClientsTest : public ::testing::Test {
public:
    UpnpClientsTest() = default;
    ~UpnpClientsTest() override = default;

    void SetUp() override
    {
        config = std::make_shared<MyConfigMock>();

        auto clientConfig = std::make_shared<ClientConfig>(123, "default", "192.168.1.100", "added by config",
            std::map<std::string, std::string>(), std::map<std::string, std::string>(), std::map<ClientMatchType, std::string>(), 1, -1, false, true);
        config->list->add(clientConfig, 0);

        auto rangeConfig = std::make_shared<ClientConfig>(456, "range", "192.168.2.0/24", "added by config",
            std::map<std::string, std::string>(), std::map<std::string, std::string>(), std::map<ClientMatchType, std::string>(), 1, -1, false, true);
        config->list->add(rangeConfig, 1);

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
    const ClientObservation* pClient;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pClient = subject->getInfo(addr, "Android/8.0.0 UPnP/1.0 BubbleUPnP/3.4.4");
    EXPECT_EQ(pClient->pInfo->type, ClientType::BubbleUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pClient = subject->getInfo(addr, "BubbleUPnP UPnP/1.1");
    EXPECT_EQ(pClient->pInfo->type, ClientType::BubbleUPnP);
}

TEST_F(UpnpClientsTest, foobar2000V1_6_2)
{
    const ClientObservation* pClient;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pClient = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pClient = subject->getInfo(addr, "foobar2000/1.6.2");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, kodiV18_9)
{
    const ClientObservation* pClient;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pClient = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Kodi");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pClient = subject->getInfo(addr, "Kodi/18.9 (Windows NT 10.0.19041; Win64; x64) App_Bitness/64 Version/18.9-(18.9.0)-Git:20201023-0655c2c718");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, samsungTVQ70)
{
    const ClientObservation* pClient;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pClient = subject->getInfo(addr, "DLNADOC/1.50 SEC_HHP_[TV] Samsung Q70 Series/1.0 UPnP/1.0");
    EXPECT_EQ(pClient->pInfo->type, ClientType::SamsungSeriesQ);

    // 2. via fileInfo (e.g. info/open/read video)
    pClient = subject->getInfo(addr, "samsung-agent/1.1");
    EXPECT_EQ(pClient->pInfo->type, ClientType::SamsungSeriesQ);
}

TEST_F(UpnpClientsTest, vlcV3_0_11_1)
{
    const ClientObservation* pClient;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    pClient = subject->getInfo(addr, "Linux/5.9.0-5-amd64, UPnP/1.0, Portable SDK for UPnP devices/1.8.4");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    pClient = subject->getInfo(addr, "VLC/3.0.11.1 LibVLC/3.0.11.1");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, windows10)
{
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // via actionReq (e.g. doBrowse)
    const ClientObservation* pClient = subject->getInfo(addr, "Microsoft-Windows/10.0 UPnP/1.0 Microsoft-DLNA DLNADOC/1.50");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);
}

TEST_F(UpnpClientsTest, multipleClientsOnSameIP)
{
    const ClientObservation* pClient;
    auto addr = std::make_shared<GrbNet>("192.168.1.42");

    // 1. Foobar2000 via actionReq (e.g. doBrowse)
    pClient = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);

    // 2. Kodi via actionReq (e.g. doBrowse)
    pClient = subject->getInfo(addr, "UPnP/1.0 DLNADOC/1.50 Kodi");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);

    // 3. Foobar2000 via fileInfo (e.g. info/open/read video)
    pClient = subject->getInfo(addr, "foobar2000/1.6.2");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);

    // 4. Kodi via fileInfo (e.g. info/open/read video)
    pClient = subject->getInfo(addr, "Kodi/18.9 (Windows NT 10.0.19041; Win64; x64) App_Bitness/64 Version/18.9-(18.9.0)-Git:20201023-0655c2c718");
    EXPECT_EQ(pClient->pInfo->type, ClientType::StandardUPnP);
}

// keep this at the end of all tests (otherwise we need a removeClientProfile function...)
TEST_F(UpnpClientsTest, configuredIP)
{
    auto addr = std::make_shared<GrbNet>("192.168.1.100");

    // act
    const ClientObservation* pClient = subject->getInfo(addr, "any unknown user-agent info");
    EXPECT_EQ(pClient->pInfo->type, ClientType::Custom);
    EXPECT_EQ(pClient->pInfo->flags, 123);
}

TEST_F(UpnpClientsTest, configuredIPRange)
{
    auto addr = std::make_shared<GrbNet>("192.168.2.100");

    // act
    const ClientObservation* pClient = subject->getInfo(addr, "any unknown user-agent info");
    EXPECT_EQ(pClient->pInfo->type, ClientType::Custom);
    EXPECT_EQ(pClient->pInfo->flags, 456);
}

TEST_F(UpnpClientsTest, getHostName)
{
    auto addr = std::make_shared<GrbNet>("192.168.99.100");
    auto hostname = addr->getHostName();
    EXPECT_EQ(hostname, "192.168.99.100");
    addr->setHostName("Wombat");
    hostname = addr->getHostName();
    EXPECT_EQ(hostname, "Wombat");
}

TEST_F(UpnpClientsTest, netmask4)
{
    auto addr = GrbNet("192.168.2.100");
    EXPECT_EQ(addr.equals("192.0.0.0/8"), true);
    EXPECT_EQ(addr.equals("192.168.0.0/16"), true);
    EXPECT_EQ(addr.equals("192.168.2.0/24"), true);
    EXPECT_EQ(addr.equals("192.168.2.100/32"), true);
    EXPECT_EQ(addr.equals("192.168.2.100"), true);
    EXPECT_EQ(addr.equals("192.168.2.101"), false);
    EXPECT_EQ(addr.equals("192.168.3.0/24"), false);
}

TEST_F(UpnpClientsTest, netmask6)
{
    auto addr = GrbNet("fe80::523e:aaff:abcd:c276");
    EXPECT_EQ(addr.equals("fe80:0:0:0:523e:aaff:abcd:c277/64"), true);
    EXPECT_EQ(addr.equals("fe80:0:0:0:523f:aaff:abcd:c277/64"), true);
    EXPECT_EQ(addr.equals("fe80:0:0:0:523e:ff:abcd:c277/64"), true);
    EXPECT_EQ(addr.equals("fe80:0:0:0:623e:aff:abcd:c276/32"), true);
    EXPECT_EQ(addr.equals("fe80:0:0001:623e:aff:abcd:c276/32"), false);
    EXPECT_EQ(addr.equals("fe80:1:0:0:523e:aaff:abcd:c276/16"), true);
    EXPECT_EQ(addr.equals("fe80:0:0:0:522e:aaff:abcd:c276/40"), true);
    EXPECT_EQ(addr.equals("fe80:0:0001:0:522e:aaff:abcd:c276/48"), false);
    EXPECT_EQ(addr.equals("fe80:0:0:0:523e:aaff:abcd:c277"), false);
    EXPECT_EQ(addr.equals("fe80:0:0:1:522e:aaff:abcd:c276/64"), false);
}

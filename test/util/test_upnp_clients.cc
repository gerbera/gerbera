#include <gtest/gtest.h>
#include <upnp.h>

#include "util/upnp_clients.h"

using namespace ::testing;

static void fillAddr(struct sockaddr_storage* addr, const char* ip)
{
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(ip);
    memcpy(addr, &sin, sizeof (sin));
}

TEST(UpnpClientsTest, bubbleUPnPV3_4_4)
{
    const ClientInfo* pInfo = nullptr;
    struct sockaddr_storage addr;
    fillAddr(&addr, "192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "Android/8.0.0 UPnP/1.0 BubbleUPnP/3.4.4", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::BubbleUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    Clients::getInfo(&addr, "BubbleUPnP UPnP/1.1", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::BubbleUPnP);
}

TEST(UpnpClientsTest, foobar2000V1_6_2)
{
    const ClientInfo* pInfo = nullptr;
    struct sockaddr_storage addr;
    fillAddr(&addr, "192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    Clients::getInfo(&addr, "foobar2000/1.6.2", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST(UpnpClientsTest, kodiV18_9)
{
    const ClientInfo* pInfo = nullptr;
    struct sockaddr_storage addr;
    fillAddr(&addr, "192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "UPnP/1.0 DLNADOC/1.50 Kodi", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    Clients::getInfo(&addr, "Kodi/18.9 (Windows NT 10.0.19041; Win64; x64) App_Bitness/64 Version/18.9-(18.9.0)-Git:20201023-0655c2c718", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST(UpnpClientsTest, samsungTVQ70)
{
    const ClientInfo* pInfo = nullptr;
    struct sockaddr_storage addr;
    fillAddr(&addr, "192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "DLNADOC/1.50 SEC_HHP_[TV] Samsung Q70 Series/1.0 UPnP/1.0", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::SamsungSeriesQ);

    // 2. via fileInfo (e.g. info/open/read video)
    Clients::getInfo(&addr, "samsung-agent/1.1", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::SamsungSeriesQ);
}

TEST(UpnpClientsTest, vlcV3_0_11_1)
{
    const ClientInfo* pInfo = nullptr;
    struct sockaddr_storage addr;
    fillAddr(&addr, "192.168.1.42");

    // 1. via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "Linux/5.9.0-5-amd64, UPnP/1.0, Portable SDK for UPnP devices/1.8.4", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. via fileInfo (e.g. info/open/read video)
    Clients::getInfo(&addr, "VLC/3.0.11.1 LibVLC/3.0.11.1", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST(UpnpClientsTest, windows10)
{
    const ClientInfo* pInfo = nullptr;
    struct sockaddr_storage addr;
    fillAddr(&addr, "192.168.1.42");

    // via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "Microsoft-Windows/10.0 UPnP/1.0 Microsoft-DLNA DLNADOC/1.50", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

TEST(UpnpClientsTest, multipleClientsOnSameIP)
{
    const ClientInfo* pInfo = nullptr;
    struct sockaddr_storage addr;
    fillAddr(&addr, "192.168.1.42");

    // 1. Foobar2000 via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 2. Kodi via actionReq (e.g. doBrowse)
    Clients::getInfo(&addr, "UPnP/1.0 DLNADOC/1.50 Kodi", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 3. Foobar2000 via fileInfo (e.g. info/open/read video)
    Clients::getInfo(&addr, "foobar2000/1.6.2", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);

    // 4. Kodi via fileInfo (e.g. info/open/read video)
    Clients::getInfo(&addr, "Kodi/18.9 (Windows NT 10.0.19041; Win64; x64) App_Bitness/64 Version/18.9-(18.9.0)-Git:20201023-0655c2c718", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::StandardUPnP);
}

// keep this at the end of all tests (otherwise we need a removeClientInfo function...)
TEST(UpnpClientsTest, configuredIP)
{
    const ClientInfo* pInfo = nullptr;
    std::string ip = "192.168.1.42";
    struct sockaddr_storage addr;
    fillAddr(&addr, ip.c_str());

    auto clientInfo = std::make_shared<struct ClientInfo>();
    clientInfo->name = "added by config";
    clientInfo->type = ClientType::SamsungAllShare;
    clientInfo->flags = QUIRK_FLAG_NONE;
    clientInfo->matchType = ClientMatchType::IP;
    clientInfo->match = ip;
    Clients::addClientInfo(clientInfo);

    // act
    Clients::getInfo(&addr, "any unknown user-agent info", &pInfo);
    EXPECT_EQ(pInfo->type, ClientType::SamsungAllShare);
}

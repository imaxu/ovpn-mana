#include <gtest/gtest.h>
#include "core/command_templates.hpp"
#include "core/openvpn_manager.hpp"
#include <string>

using namespace ovpn::commands;

TEST(CommandTemplatesTest, ReplaceConfigPlaceholders)
{
    AppConfig cfg;
    cfg.easy_rsa_dir = "/test/easy-rsa";
    cfg.ovpn_dir = "/test/openvpn";
    cfg.openvpn_bin = "/usr/sbin/openvpn";
    cfg.systemctl_bin = "/bin/systemctl";

    std::string result = replace(EASYRSA_GEN_REQ, cfg);
    EXPECT_NE(result.find("/test/easy-rsa"), std::string::npos);
    EXPECT_EQ(result.find("{EASY_RSA_DIR}"), std::string::npos);
}

TEST(CommandTemplatesTest, ReplaceBusinessParams)
{
    AppConfig cfg = AppConfig::defaults();
    std::string result = replace(EASYRSA_BUILD_CLIENT_FULL, cfg, {{"NAME", "test-client"}});
    EXPECT_NE(result.find("test-client"), std::string::npos);
    EXPECT_EQ(result.find("{NAME}"), std::string::npos);
}

TEST(CommandTemplatesTest, ReplaceMixed)
{
    AppConfig cfg;
    cfg.easy_rsa_dir = "/opt/easy-rsa";
    std::string result = replace(CP_WITH_SUDO, cfg, {{"SRC", "/src/ca.crt"}, {"DEST", "/dest/ca.crt"}});
    EXPECT_NE(result.find("sudo cp"), std::string::npos);
    EXPECT_NE(result.find("/src/ca.crt"), std::string::npos);
    EXPECT_NE(result.find("/dest/ca.crt"), std::string::npos);
    EXPECT_EQ(result.find("{SRC}"), std::string::npos);
    EXPECT_EQ(result.find("{DEST}"), std::string::npos);
}

TEST(CommandTemplatesTest, UnmatchedPlaceholderPreserved)
{
    AppConfig cfg = AppConfig::defaults();
    std::string result = replace("{UNKNOWN} {NAME}", cfg);
    EXPECT_NE(result.find("{UNKNOWN}"), std::string::npos);
}

TEST(CommandTemplatesTest, ServerConfigTemplate)
{
    AppConfig cfg;
    cfg.ovpn_server_conf_dir();
    std::string result = replace(SERVER_CONFIG, cfg, {
        {"PORT", "1194"},
        {"SUBNET", "10.8.0.0 255.255.255.0"},
        {"SERVER_DIR", "/etc/openvpn/server/test"}
    });

    EXPECT_NE(result.find("port 1194"), std::string::npos);
    EXPECT_NE(result.find("server 10.8.0.0 255.255.255.0"), std::string::npos);
    EXPECT_NE(result.find("ca /etc/openvpn/server/test/ca.crt"), std::string::npos);
    EXPECT_EQ(result.find("{"), std::string::npos);
}

TEST(CommandTemplatesTest, ClientConfigTemplate)
{
    AppConfig cfg = AppConfig::defaults();
    std::string result = replace(CLIENT_CONFIG, cfg, {
        {"WAN_IP", "1.2.3.4"},
        {"PORT", "1194"}
    });

    EXPECT_NE(result.find("remote 1.2.3.4 1194"), std::string::npos);
    EXPECT_NE(result.find("client"), std::string::npos);
    EXPECT_NE(result.find("dev tun"), std::string::npos);
    EXPECT_EQ(result.find("{"), std::string::npos);
}

TEST(CommandTemplatesTest, DefaultConfigWorks)
{
    AppConfig def = AppConfig::defaults();
    std::string result = replace(SYSTEMCTL_START, def, {{"NAME", "test-svc"}});
    EXPECT_NE(result.find("/bin/systemctl start openvpn@test-svc"), std::string::npos);
}
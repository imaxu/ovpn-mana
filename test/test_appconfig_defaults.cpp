#include <gtest/gtest.h>
#include "core/openvpn_manager.hpp"
#include "core/command_templates.hpp"

TEST(AppConfigDefaultsTest, DefaultValuesMatchExpected)
{
    AppConfig cfg = AppConfig::defaults();

    EXPECT_EQ(cfg.easy_rsa_dir, "/etc/openvpn/easy-rsa");
    EXPECT_EQ(cfg.ovpn_dir, "/etc/openvpn");
    EXPECT_EQ(cfg.openvpn_bin, "/usr/sbin/openvpn");
    EXPECT_EQ(cfg.systemctl_bin, "/bin/systemctl");
}

TEST(AppConfigDefaultsTest, DerivedPathsAreConsistent)
{
    AppConfig cfg = AppConfig::defaults();

    EXPECT_EQ(cfg.ovpn_server_conf_dir(), cfg.ovpn_dir + "/server");
    EXPECT_EQ(cfg.client_configs_dir(), cfg.ovpn_dir + "/client-configs");
}

TEST(AppConfigDefaultsTest, DefaultCommandNoReplacements)
{
    AppConfig cfg = AppConfig::defaults();

    std::string result = ovpn::commands::replace(ovpn::commands::EASYRSA_GEN_REQ, cfg);
    EXPECT_EQ(result, "cd /etc/openvpn/easy-rsa && easyrsa --batch gen-req {NAME}");

    result = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_START, cfg);
    EXPECT_EQ(result, "/bin/systemctl start openvpn@{NAME}");

    result = ovpn::commands::replace(ovpn::commands::OPENVPN_GEN_TA_KEY, cfg);
    EXPECT_EQ(result, "sudo /usr/sbin/openvpn --genkey secret {OUTPUT_PATH}");
}

TEST(AppConfigDefaultsTest, CustomConfigOverridesDefaults)
{
    AppConfig cfg;
    cfg.easy_rsa_dir = "/custom/easy-rsa";
    cfg.ovpn_dir = "/custom/openvpn";
    cfg.openvpn_bin = "/custom/openvpn";
    cfg.systemctl_bin = "/custom/systemctl";

    std::string result = ovpn::commands::replace(ovpn::commands::EASYRSA_GEN_REQ, cfg);
    EXPECT_NE(result.find("/custom/easy-rsa"), std::string::npos);
    EXPECT_EQ(result.find("/home/xuwh"), std::string::npos);

    result = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_START, cfg);
    EXPECT_NE(result.find("/custom/systemctl"), std::string::npos);
}

TEST(AppConfigDefaultsTest, ConfigurationIsIdempotent)
{
    AppConfig a = AppConfig::defaults();
    AppConfig b = AppConfig::defaults();

    EXPECT_EQ(a.easy_rsa_dir, b.easy_rsa_dir);
    EXPECT_EQ(a.ovpn_dir, b.ovpn_dir);
    EXPECT_EQ(a.openvpn_bin, b.openvpn_bin);
    EXPECT_EQ(a.systemctl_bin, b.systemctl_bin);
    EXPECT_EQ(a.ovpn_server_conf_dir(), b.ovpn_server_conf_dir());
    EXPECT_EQ(a.client_configs_dir(), b.client_configs_dir());
}
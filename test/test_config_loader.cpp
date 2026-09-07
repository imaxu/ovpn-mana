#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "core/config_loader.hpp"

namespace fs = std::filesystem;

class ConfigLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "ovpn_mana_test";
        fs::create_directories(test_dir_);
        config_path_ = (test_dir_ / "test-config.json").string();
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    void writeConfig(const std::string& content) {
        std::ofstream file(config_path_);
        file << content;
        file.close();
    }

    fs::path test_dir_;
    std::string config_path_;
};

TEST_F(ConfigLoaderTest, LoadDefaultConfig) {
    auto config = ovpn::config::load_from_file("nonexistent.json");
    
    EXPECT_EQ(config.easy_rsa_dir, "/etc/openvpn/easy-rsa");
    EXPECT_EQ(config.ovpn_dir, "/etc/openvpn");
    EXPECT_EQ(config.openvpn_bin, "/usr/sbin/openvpn");
    EXPECT_EQ(config.systemctl_bin, "/bin/systemctl");
}

TEST_F(ConfigLoaderTest, LoadCustomConfig) {
    writeConfig(R"({
        "easy_rsa_dir": "/custom/easy-rsa",
        "ovpn_dir": "/custom/openvpn",
        "openvpn_bin": "/usr/local/bin/openvpn",
        "systemctl_bin": "/usr/bin/systemctl"
    })");

    auto config = ovpn::config::load_from_file(config_path_);
    
    EXPECT_EQ(config.easy_rsa_dir, "/custom/easy-rsa");
    EXPECT_EQ(config.ovpn_dir, "/custom/openvpn");
    EXPECT_EQ(config.openvpn_bin, "/usr/local/bin/openvpn");
    EXPECT_EQ(config.systemctl_bin, "/usr/bin/systemctl");
}

TEST_F(ConfigLoaderTest, PartialConfigOverride) {
    writeConfig(R"({
        "easy_rsa_dir": "/partial/easy-rsa"
    })");

    auto config = ovpn::config::load_from_file(config_path_);
    
    EXPECT_EQ(config.easy_rsa_dir, "/partial/easy-rsa");
    EXPECT_EQ(config.ovpn_dir, "/etc/openvpn");
    EXPECT_EQ(config.openvpn_bin, "/usr/sbin/openvpn");
    EXPECT_EQ(config.systemctl_bin, "/bin/systemctl");
}

TEST_F(ConfigLoaderTest, EmptyConfigFile) {
    writeConfig("{}");
    
    auto config = ovpn::config::load_from_file(config_path_);
    
    EXPECT_EQ(config.easy_rsa_dir, "/etc/openvpn/easy-rsa");
    EXPECT_EQ(config.ovpn_dir, "/etc/openvpn");
}

TEST_F(ConfigLoaderTest, EnvVarOverride) {
    writeConfig(R"({
        "easy_rsa_dir": "/file/easy-rsa",
        "ovpn_dir": "/file/openvpn"
    })");

#ifdef _WIN32
    _putenv_s("OVPN_EASY_RSA_DIR", "/env/easy-rsa");
    _putenv_s("OVPN_DIR", "/env/openvpn");
#else
    setenv("OVPN_EASY_RSA_DIR", "/env/easy-rsa", 1);
    setenv("OVPN_DIR", "/env/openvpn", 1);
#endif

    auto config = ovpn::config::load_from_file(config_path_);
    config = ovpn::config::load_with_env_override(config);
    
    EXPECT_EQ(config.easy_rsa_dir, "/env/easy-rsa");
    EXPECT_EQ(config.ovpn_dir, "/env/openvpn");
    EXPECT_EQ(config.openvpn_bin, "/usr/sbin/openvpn");

#ifdef _WIN32
    _putenv_s("OVPN_EASY_RSA_DIR", "");
    _putenv_s("OVPN_DIR", "");
#else
    unsetenv("OVPN_EASY_RSA_DIR");
    unsetenv("OVPN_DIR");
#endif
}

TEST_F(ConfigLoaderTest, LoadConfigIntegration) {
    writeConfig(R"({
        "easy_rsa_dir": "/integrated/easy-rsa"
    })");

    auto config = ovpn::config::load_config(config_path_);
    
    EXPECT_EQ(config.easy_rsa_dir, "/integrated/easy-rsa");
    EXPECT_EQ(config.ovpn_dir, "/etc/openvpn");
}
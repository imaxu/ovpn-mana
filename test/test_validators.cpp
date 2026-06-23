#include "core/validators.hpp"
#include <gtest/gtest.h>

using namespace ovpn::validators;

class ValidatorTest : public ::testing::Test {};

TEST_F(ValidatorTest, ValidateName_ValidNamesPass) {
    EXPECT_TRUE(validateName("test").valid);
    EXPECT_TRUE(validateName("test123").valid);
    EXPECT_TRUE(validateName("my_client").valid);
    EXPECT_TRUE(validateName("client-01").valid);
    EXPECT_TRUE(validateName("vpn.server").valid);
    EXPECT_TRUE(validateName("a").valid);
    EXPECT_TRUE(validateName("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_.").valid);
}

TEST_F(ValidatorTest, ValidateName_EmptyFails) {
    EXPECT_FALSE(validateName("").valid);
}

TEST_F(ValidatorTest, ValidateName_TooLongFails) {
    std::string longName(65, 'a');
    EXPECT_FALSE(validateName(longName).valid);
}

TEST_F(ValidatorTest, ValidateName_ShellMetacharsFails) {
    EXPECT_FALSE(validateName("test;rm").valid);
    EXPECT_FALSE(validateName("test|cat").valid);
    EXPECT_FALSE(validateName("test&sleep").valid);
    EXPECT_FALSE(validateName("test`whoami`").valid);
    EXPECT_FALSE(validateName("test'echo").valid);
    EXPECT_FALSE(validateName("test\"rm").valid);
}

TEST_F(ValidatorTest, ValidateName_PathTraversalFails) {
    EXPECT_FALSE(validateName("../etc/passwd").valid);
    EXPECT_FALSE(validateName("./test").valid);
}

TEST_F(ValidatorTest, ValidateName_BadStartFails) {
    EXPECT_FALSE(validateName("-test").valid);
    EXPECT_FALSE(validateName("_test").valid);
}

TEST_F(ValidatorTest, ValidateName_NonPrintableFails) {
    std::string name;
    name += "test";
    name += static_cast<char>(0x01);
    EXPECT_FALSE(validateName(name).valid);
}

TEST_F(ValidatorTest, ValidateServiceName_RejectsServerSuffix) {
    EXPECT_FALSE(validateServiceName("myvpn-server").valid);
    EXPECT_TRUE(validateServiceName("myvpn").valid);
    EXPECT_TRUE(validateServiceName("server-client").valid);
}

TEST_F(ValidatorTest, ValidateIPv4_ValidPasses) {
    EXPECT_TRUE(validateIPv4("192.168.1.1").valid);
    EXPECT_TRUE(validateIPv4("10.0.0.1").valid);
    EXPECT_TRUE(validateIPv4("8.8.8.8").valid);
    EXPECT_TRUE(validateIPv4("0.0.0.0").valid);
    EXPECT_TRUE(validateIPv4("255.255.255.255").valid);
}

TEST_F(ValidatorTest, ValidateIPv4_OutOfRangeFails) {
    EXPECT_FALSE(validateIPv4("256.0.0.1").valid);
    EXPECT_FALSE(validateIPv4("192.168.1.256").valid);
}

TEST_F(ValidatorTest, ValidateIPv4_LeadingZeroFails) {
    EXPECT_FALSE(validateIPv4("192.168.01.1").valid);
}

TEST_F(ValidatorTest, ValidateIPv4_WrongSegmentsFails) {
    EXPECT_FALSE(validateIPv4("192.168.1").valid);
    EXPECT_FALSE(validateIPv4("192.168.1.1.1").valid);
}

TEST_F(ValidatorTest, ValidateSubnet_ValidPasses) {
    EXPECT_TRUE(validateSubnet("192.168.1.0/24").valid);
    EXPECT_TRUE(validateSubnet("10.0.0.0/8").valid);
    EXPECT_TRUE(validateSubnet("172.16.0.0/12").valid);
}

TEST_F(ValidatorTest, ValidateSubnet_NonZeroHostFails) {
    EXPECT_FALSE(validateSubnet("192.168.1.1/24").valid);
}

TEST_F(ValidatorTest, ValidateSubnet_BadPrefixFails) {
    EXPECT_FALSE(validateSubnet("192.168.1.0/7").valid);
    EXPECT_FALSE(validateSubnet("192.168.1.0/31").valid);
}

TEST_F(ValidatorTest, ValidatePort_ValidPasses) {
    EXPECT_TRUE(validatePort(1024).valid);
    EXPECT_TRUE(validatePort(1194).valid);
    EXPECT_TRUE(validatePort(65535).valid);
}

TEST_F(ValidatorTest, ValidatePort_OutOfRangeFails) {
    EXPECT_FALSE(validatePort(1023).valid);
    EXPECT_FALSE(validatePort(65536).valid);
    EXPECT_FALSE(validatePort(0).valid);
}
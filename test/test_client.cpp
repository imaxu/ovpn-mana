#include "OpenVPNManager.hpp"
#include <gtest/gtest.h>

class OpenVPNTest : public ::testing::Test {
protected:
    void SetUp() override {
        testClient = "test_client_" + std::to_string(rand());
    }
    std::string testClient;
};

// TEST_F(OpenVPNTest, ClientLifecycle) {
//     EXPECT_TRUE(OpenVPNManager::createClient(testClient));
//     EXPECT_TRUE(OpenVPNManager::revokeClient(testClient));
// }

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
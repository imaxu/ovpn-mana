#include <gtest/gtest.h>
#include "ovpn-mana/ovpn_mana_api.h"
#include "ovpn-mana/ovpn_mana_types.h"

TEST(ValidatorIntegrationTest, CreateServiceRejectsShellInjection)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_service(handle, "test;rm -rf /", "10.8.0.0", 1194);
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, CreateServiceRejectsPathTraversal)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_service(handle, "../etc", "10.8.0.0", 1194);
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, CreateClientRejectsInvalidIP)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_client_with_ip(handle, "test-svc", "test-cli", "1.2.3.4", "999.999.999.999");
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, CreateServiceRejectsBadPort)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_service(handle, "test-svc", "10.8.0.0/24", 0);
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    err = ovpn_mana_create_service(handle, "test-svc", "10.8.0.0/24", 80);
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, CreateServiceRejectsServerSuffix)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_service(handle, "my-server", "10.8.0.0", 1194);
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, CreateServiceRejectsEmptyName)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_service(handle, "", "10.8.0.0", 1194);
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, CreateClientRejectsEmptyName)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_client(handle, "test-svc", "", "1.2.3.4");
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, CreateClientRejectsInvalidServiceName)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_create_client(handle, "bad;name", "test-cli", "1.2.3.4");
    EXPECT_NE(err, OVPN_ERR_SUCCESS);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, ConfigureRejectsNullHandle)
{
    ovpn_config_t cfg = {};
    ovpn_err_t err = ovpn_mana_configure(nullptr, &cfg);
    EXPECT_EQ(err, OVPN_ERR_INVALID_PARAM);
}

TEST(ValidatorIntegrationTest, ConfigureRejectsNullConfig)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    ovpn_err_t err = ovpn_mana_configure(handle, nullptr);
    EXPECT_EQ(err, OVPN_ERR_INVALID_PARAM);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, ExportClientConfigRejectsNullHandle)
{
    int bufSize = 0;
    ovpn_err_t err = ovpn_mana_export_client_config(nullptr, "test", "test", nullptr, bufSize);
    EXPECT_EQ(err, OVPN_ERR_INVALID_PARAM);
}

TEST(ValidatorIntegrationTest, ExportClientConfigRejectsNullServiceName)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    int bufSize = 0;
    ovpn_err_t err = ovpn_mana_export_client_config(handle, nullptr, "test", nullptr, bufSize);
    EXPECT_EQ(err, OVPN_ERR_INVALID_PARAM);

    ovpn_mana_destroy(handle);
}

TEST(ValidatorIntegrationTest, ExportClientConfigRejectsInvalidServiceName)
{
    ovpn_mana_handle_t handle = ovpn_mana_create();
    ASSERT_NE(handle, nullptr);

    int bufSize = 0;
    ovpn_err_t err = ovpn_mana_export_client_config(handle, "bad;svc", "test", nullptr, bufSize);
    EXPECT_EQ(err, OVPN_ERR_INVALID_PARAM);

    ovpn_mana_destroy(handle);
}
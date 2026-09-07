#include "ovpn-mana/ovpn_mana_api.h"
#include "core/openvpn_manager.hpp"
#include "core/validators.hpp"
#include "core/config_loader.hpp"
#include "ovpn-mana/ovpn_mana_version.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <memory>

LIB_API ovpn_mana_handle_t LIB_API_CALL ovpn_mana_create()
{
  try
  {
    OpenVPNManager *manager = new OpenVPNManager();
    AppConfig config = ovpn::config::load_config();

    std::cerr << "[CONFIG] Loading configuration..." << std::endl;
    std::cerr << "[CONFIG] Easy-RSA Dir: " << config.easy_rsa_dir << std::endl;
    std::cerr << "[CONFIG] OpenVPN Dir: " << config.ovpn_dir << std::endl;
    std::cerr << "[CONFIG] OpenVPN Bin: " << config.openvpn_bin << std::endl;
    std::cerr << "[CONFIG] Systemctl Bin: " << config.systemctl_bin << std::endl;

    manager->configure(config);
    return reinterpret_cast<ovpn_mana_handle_t>(manager);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to create OpenVPN manager: " << e.what() << std::endl;
    return nullptr;
  }
}

LIB_API void LIB_API_CALL ovpn_mana_destroy(ovpn_mana_handle_t handle)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    delete manager;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to destroy OpenVPN manager: " << e.what() << std::endl;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_list_services(ovpn_mana_handle_t handle, ovpn_service_t *services, int &service_count)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::vector<VPNService> service_list = manager->listServices();
    service_count = service_list.size();

    if (service_count > 0)
    {
      for (int i = 0; i < service_count; ++i)
      {
        strncpy(services[i].name, service_list[i].name.c_str(), sizeof(services[i].name));
        strncpy(services[i].configPath, service_list[i].configPath.c_str(), sizeof(services[i].configPath));
        services[i].port = service_list[i].port;
        strncpy(services[i].subnet, service_list[i].subnet.c_str(), sizeof(services[i].subnet));
        services[i].is_activated = service_list[i].isActive;
        services[i].is_enabled = service_list[i].isEnabled;
      }
    }

    return OVPN_ERR_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to list OpenVPN services: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_service(ovpn_mana_handle_t handle, const char *name, const char* subnet, int port)
{
  if (handle == nullptr || name == nullptr || subnet == nullptr) {
    return OVPN_ERR_INVALID_PARAM;
  }

  auto nameResult = ovpn::validators::validateServiceName(name);
  if (!nameResult.valid) {
    std::cerr << "Invalid service name '" << name << "': " << nameResult.reason << std::endl;
    return OVPN_ERR_INVALID_PARAM;
  }
  auto subnetResult = ovpn::validators::validateSubnet(subnet);
  if (!subnetResult.valid) {
    std::cerr << "Invalid subnet '" << subnet << "': " << subnetResult.reason << std::endl;
    return OVPN_ERR_INVALID_PARAM;
  }
  auto portResult = ovpn::validators::validatePort(port);
  if (!portResult.valid) {
    std::cerr << "Invalid port " << port << ": " << portResult.reason << std::endl;
    return OVPN_ERR_PORT_RANGE;
  }

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->createService(name, subnet, port))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to create OpenVPN service: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_start_service(ovpn_mana_handle_t handle, const char *name)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->startService(name))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to start OpenVPN service: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_stop_service(ovpn_mana_handle_t handle, const char *name)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->stopService(name))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to stop OpenVPN service: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_restart_service(ovpn_mana_handle_t handle, const char *name)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->restartService(name))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to restart OpenVPN service: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_delete_service(ovpn_mana_handle_t handle, const char *name)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->deleteService(name))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to delete OpenVPN service: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip)
{
  if (handle == nullptr || service_name == nullptr || name == nullptr || wanip == nullptr) {
    return OVPN_ERR_INVALID_PARAM;
  }

  auto clientNameResult = ovpn::validators::validateClientName(name);
  if (!clientNameResult.valid) {
    std::cerr << "Invalid client name '" << name << "': " << clientNameResult.reason << std::endl;
    return OVPN_ERR_INVALID_PARAM;
  }
  auto ipResult = ovpn::validators::validateHostOrIP(wanip);
  if (!ipResult.valid) {
    std::cerr << "Invalid WAN address '" << wanip << "': " << ipResult.reason << std::endl;
    return OVPN_ERR_IP_FORMAT;
  }

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->createClient(name, service_name, wanip))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to create OpenVPN client: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client_with_ip(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip, const char* client_ip)
{
  if (handle == nullptr || service_name == nullptr || name == nullptr || wanip == nullptr || client_ip == nullptr) {
    return OVPN_ERR_INVALID_PARAM;
  }

  auto clientNameResult = ovpn::validators::validateClientName(name);
  if (!clientNameResult.valid) {
    std::cerr << "Invalid client name '" << name << "': " << clientNameResult.reason << std::endl;
    return OVPN_ERR_INVALID_PARAM;
  }
  auto ipResult = ovpn::validators::validateHostOrIP(wanip);
  if (!ipResult.valid) {
    std::cerr << "Invalid WAN address '" << wanip << "': " << ipResult.reason << std::endl;
    return OVPN_ERR_IP_FORMAT;
  }

  if (client_ip[0] != '\0') {
    auto clientIpResult = ovpn::validators::validateIPv4(client_ip);
    if (!clientIpResult.valid) {
      std::cerr << "Invalid client IP '" << client_ip << "': " << clientIpResult.reason << std::endl;
      return OVPN_ERR_IP_FORMAT;
    }
  }

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->createClient(name, service_name, wanip, client_ip))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to create OpenVPN client with fixed IP: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_export_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name, char *buffer, int &buffer_size)
{
  if (handle == nullptr || service_name == nullptr || name == nullptr) {
    return OVPN_ERR_INVALID_PARAM;
  }

  auto nameResult = ovpn::validators::validateServiceName(service_name);
  if (!nameResult.valid) {
    std::cerr << "Invalid service name '" << service_name << "': " << nameResult.reason << std::endl;
    return OVPN_ERR_INVALID_PARAM;
  }
  auto clientNameResult = ovpn::validators::validateClientName(name);
  if (!clientNameResult.valid) {
    std::cerr << "Invalid client name '" << name << "': " << clientNameResult.reason << std::endl;
    return OVPN_ERR_INVALID_PARAM;
  }

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::string content = manager->getOVPNFileContent(name, service_name);
    if (content.empty()) {
      return OVPN_ERR_CLIENT_NOT_FOUND;
    }

    if (buffer == nullptr) {
      buffer_size = (int)(content.size() + 1);
      return OVPN_ERR_SUCCESS;
    }

    if (buffer_size < (int)(content.size() + 1)) {
      buffer_size = (int)(content.size() + 1);
      return OVPN_ERR_BUFFER_TOO_SMALL;
    }

    std::strncpy(buffer, content.c_str(), content.size() + 1);
    buffer_size = (int)(content.size() + 1);
    return OVPN_ERR_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to export client config: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->revokeClient(name, service_name))
    {
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to revoke OpenVPN client: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_online_clients(ovpn_mana_handle_t handle, const char *service_name, ovpn_client_t *clients, int &client_count)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::vector<VPNClient> client_list = manager->getOnlineClients(service_name);
    client_count = client_list.size();

    if (clients != nullptr && client_count > 0)
    {
      for (int i = 0; i < client_count; ++i)
      {
        snprintf(clients[i].name, sizeof(clients[i].name), "%s", client_list[i].name.c_str());
        snprintf(clients[i].private_ipv4, sizeof(clients[i].private_ipv4), "%s", client_list[i].vpnIp.c_str());
        snprintf(clients[i].public_ipv4, sizeof(clients[i].public_ipv4), "%s", client_list[i].realIp.c_str());
        snprintf(clients[i].since, sizeof(clients[i].since), "%s", client_list[i].since.c_str());
        clients[i].bytes_received = client_list[i].bytesReceived;
        clients[i].bytes_sent = client_list[i].bytesSent;
      }
    }

    return OVPN_ERR_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to get online OpenVPN clients: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_total_clients_count(ovpn_mana_handle_t handle, const char *service_name, int &total_count)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    total_count = manager->getTotalClientsCount(service_name);
    return OVPN_ERR_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to get total clients count: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_total_clients(ovpn_mana_handle_t handle, const char *service_name, ovpn_client_t *clients, int &client_count)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::vector<VPNClient> client_list = manager->getTotalClients(service_name);
    client_count = client_list.size();

    if (clients != nullptr && client_count > 0)
    {
      for (int i = 0; i < client_count; ++i)
      {
        snprintf(clients[i].name, sizeof(clients[i].name), "%s", client_list[i].name.c_str());
        snprintf(clients[i].private_ipv4, sizeof(clients[i].private_ipv4), "%s", client_list[i].vpnIp.c_str());
        memset(clients[i].public_ipv4, 0, sizeof(clients[i].public_ipv4));
        memset(clients[i].since, 0, sizeof(clients[i].since));
        clients[i].bytes_received = client_list[i].bytesReceived;
        clients[i].bytes_sent = client_list[i].bytesSent;
      }
    }

    return OVPN_ERR_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to get total clients list: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name, char *ovpn_file, int &ovpn_file_size)
{
  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::string content = manager->getOVPNFileContent(name, service_name);
    ovpn_file_size = content.size();

    if (ovpn_file_size > 0)
    {
      strncpy(ovpn_file, content.c_str(), ovpn_file_size);
      return OVPN_ERR_SUCCESS;
    }
    else
    {
      return -1;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to get OpenVPN client config: " << e.what() << std::endl;
    return -1;
  }
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_configure(ovpn_mana_handle_t handle, const ovpn_config_t *config)
{
  try
  {
    if (handle == nullptr || config == nullptr) {
      return OVPN_ERR_INVALID_PARAM;
    }

    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    AppConfig cfg = AppConfig::defaults();

    if (config->easy_rsa_dir[0] != '\0') {
      cfg.easy_rsa_dir = config->easy_rsa_dir;
    }
    if (config->ovpn_dir[0] != '\0') {
      cfg.ovpn_dir = config->ovpn_dir;
    }
    if (config->openvpn_bin[0] != '\0') {
      cfg.openvpn_bin = config->openvpn_bin;
    }
    if (config->systemctl_bin[0] != '\0') {
      cfg.systemctl_bin = config->systemctl_bin;
    }

    manager->configure(cfg);
    return OVPN_ERR_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to configure OpenVPN manager: " << e.what() << std::endl;
    return OVPN_ERR_FAILURE;
  }
}

LIB_API const char* LIB_API_CALL ovpn_mana_get_version()
{
  return OVPN_VERSION_STRING;
}

LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_load_config(ovpn_mana_handle_t handle, const char *config_path)
{
  try
  {
    if (handle == nullptr || config_path == nullptr) {
      return OVPN_ERR_INVALID_PARAM;
    }

    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::string path(config_path);
    AppConfig config = ovpn::config::load_config(path);
    manager->configure(config);

    return OVPN_ERR_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to load config from file: " << e.what() << std::endl;
    return OVPN_ERR_FAILURE;
  }
}
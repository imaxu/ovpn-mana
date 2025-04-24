
#include "ovpn-mana.hpp"
#include "OpenVPNManager.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <memory>

/// @brief  创建OpenVPN管理器实例并返回句柄
/// @return  句柄
/// @note    该函数会创建一个OpenVPN管理器实例，并返回一个句柄。该句柄可以用于后续的操作。
LIB_API ovpn_mana_handle_t LIB_API_CALL ovpn_mana_create()
{
  try
  {
    OpenVPNManager *manager = new OpenVPNManager();
    return reinterpret_cast<ovpn_mana_handle_t>(manager);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to create OpenVPN manager: " << e.what() << std::endl;
    return nullptr;
  }
}

/// @brief  销毁OpenVPN管理器实例
/// @param  handle  句柄
/// @note    该函数会销毁OpenVPN管理器实例，并释放相关资源。
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

/// @brief  获取OpenVPN服务列表
/// @param  handle  句柄
/// @param  services  服务列表
/// @param  count  服务数量
/// @return  错误码
/// @note    该函数会获取OpenVPN服务列表，并返回服务数量。服务列表中的每个服务包含名称、配置路径、是否激活和是否自启等信息。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_list_services(ovpn_mana_handle_t handle, ovpn_service_t *services, int &service_count)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::vector<VPNService> service_list = manager->listServices();
    service_count = service_list.size();

    if (service_count > 0)
    {
      // 转换VPNService 到 ovpn_service_t
      for (int i = 0; i < service_count; ++i)
      {
        strncpy(services[i].name, service_list[i].name.c_str(), sizeof(services[i].name));
        strncpy(services[i].configPath, service_list[i].configPath.c_str(), sizeof(services[i].configPath));
        services[i].is_activated = service_list[i].isActive;
        services[i].is_enabled = service_list[i].isEnabled;
      }
    }

    return OVPN_ERR_SUCCESS; // 成功
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to list OpenVPN services: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  创建OpenVPN服务
/// @param  handle  句柄
/// @param  name  服务名称
/// @param  port  服务端口
/// @return  错误码
/// @note    该函数会创建一个OpenVPN服务，并返回错误码。服务名称和端口号必须合法。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_service(ovpn_mana_handle_t handle, const char *name, int port)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->createService(name, port))
    {
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to create OpenVPN service: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  启动OpenVPN服务
/// @param  handle  句柄
/// @param  name  服务名称
/// @return  错误码
/// @note    该函数会启动一个OpenVPN服务，并返回错误码。服务名称必须合法。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_start_service(ovpn_mana_handle_t handle, const char *name)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->startService(name))
    {
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to start OpenVPN service: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  停止OpenVPN服务
/// @param  handle  句柄
/// @param  name  服务名称
/// @return  错误码
/// @note    该函数会停止一个OpenVPN服务，并返回错误码。服务名称必须合法。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_stop_service(ovpn_mana_handle_t handle, const char *name)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->stopService(name))
    {
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to stop OpenVPN service: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  重启OpenVPN服务
/// @param  handle  句柄
/// @param  name  服务名称
/// @return  错误码
/// @note    该函数会重启一个OpenVPN服务，并返回错误码。服务名称必须合法。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_restart_service(ovpn_mana_handle_t handle, const char *name)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->restartService(name))
    {
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to restart OpenVPN service: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  删除OpenVPN服务
/// @param  handle  句柄
/// @param  name  服务名称
/// @return  错误码
/// @note    该函数会删除一个OpenVPN服务，并返回错误码。服务名称必须合法。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_delete_service(ovpn_mana_handle_t handle, const char *name)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->deleteService(name))
    {
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to delete OpenVPN service: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  创建OpenVPN客户端
/// @param  handle  句柄
/// @param  service_name  服务名称
/// @param  name  客户端名称
/// @return  错误码
/// @note    该函数会创建一个OpenVPN客户端，并返回错误码。客户端名称和服务名称必须合法。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name)
{
  try
  {

    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->createClient(name, service_name))
    {
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to create OpenVPN client: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  吊销OpenVPN客户端
/// @param  handle  句柄
/// @param  service_name  服务名称
/// @param  name  客户端名称
/// @return  错误码
/// @note    该函数会吊销一个OpenVPN客户端，并返回错误码。客户端名称和服务名称必须合法。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    if (manager->revokeClient(name, service_name))
    {
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to revoke OpenVPN client: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/// @brief  获取在线OpenVPN客户端列表
/// @param  handle  句柄
/// @param  service_name  服务名称
/// @param  clients  客户端列表
/// @param  client_count  客户端数量
/// @return  错误码
/// @note    该函数会获取在线OpenVPN客户端列表，并返回客户端数量。客户端列表中的每个客户端包含名称、VPN IP、真实 IP、上线时间、接收字节数和发送字节数等信息。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_online_clients(ovpn_mana_handle_t handle, const char *service_name, ovpn_client_t *clients, int &client_count)
{

  try
  {
    OpenVPNManager *manager = reinterpret_cast<OpenVPNManager *>(handle);
    std::vector<VPNClient> client_list = manager->getOnlineClients(service_name);
    client_count = client_list.size();

    if (client_count > 0)
    {
      // 转换VPNClient 到 ovpn_client_t
      for (int i = 0; i < client_count; ++i)
      {
        strncpy(clients[i].name, client_list[i].name.c_str(), sizeof(clients[i].name));
        strncpy(clients[i].private_ipv4, client_list[i].vpnIp.c_str(), sizeof(clients[i].private_ipv4));
        strncpy(clients[i].public_ipv4, client_list[i].realIp.c_str(), sizeof(clients[i].public_ipv4));
        strncpy(clients[i].since, client_list[i].since.c_str(), sizeof(clients[i].since));
        clients[i].bytes_received = client_list[i].bytesReceived;
        clients[i].bytes_sent = client_list[i].bytesSent;
      }
    }

    return OVPN_ERR_SUCCESS; // 成功
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to get online OpenVPN clients: " << e.what() << std::endl;
    return -1; // 错误
  }
}
/// @brief  返回指定OpenVPN客户端配置文件内容
/// @param  handle  句柄
/// @param  service_name  服务名称
/// @param  name  客户端名称
/// @param  ovpn_file  客户端配置文件内容
/// @param  ovpn_file_size  客户端配置文件大小
/// @return  错误码
/// @note    该函数会返回指定OpenVPN客户端配置文件内容，并返回配置文件大小。客户端名称和服务名称必须合法。
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
      return OVPN_ERR_SUCCESS; // 成功
    }
    else
    {
      return -1; // 错误
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to get OpenVPN client config: " << e.what() << std::endl;
    return -1; // 错误
  }
}

/**
 * @file main.cpp
 * @brief OpenVPN管理器主程序
 * @author xuwh (imaxu@sina.com)
 * @date 2023-10-01
 * @version 1.0
 * @note  用法： ./ovpn-mana service -l  列出服务
 * @note  用法： ./ovpn-mana service -c xxxx,1194,10.1.0.0  创建服务
 * @note  用法： ./ovpn-mana service -d xxxx 删除服务
 * @note  用法： ./ovpn-mana service -start xxxx 启动服务
 * @note  用法： ./ovpn-mana service -stop xxxx 停止服务
 * @note  用法： ./ovpn-mana service -restart xxxx 重启服务
 * @note  用法： ./ovpn-mana client -c xxxx,client1 创建客户端
 * @note  用法： ./ovpn-mana client -d xxxx,client1 吊销客户端
 * @note  用法： ./ovpn-mana client -l xxxx 列出在线客户端
 * @note  用法： ./ovpn-mana client -conf xxxx,client1 获取客户端配置文件
 */

#include <iomanip>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "ovpn-mana.hpp"

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE) || defined(WIN)
#define getuid() 0
#else
#include <unistd.h>
#endif // DEBUG


/** 服务相关命令处理函数 */

/// @brief 列出服务
/// @param handle 句柄
void print_services(ovpn_mana_handle_t handle)
{
  ovpn_service_t services[10];
  int service_count = 0;
  ovpn_err_t err = ovpn_mana_list_services(handle, services, service_count);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to list OpenVPN services" << std::endl;
    return;
  }
  std::cout << "OpenVPN Services:" << std::endl;
  for (int i = 0; i < service_count; ++i)
  {

    std::cout 
    << "  Name: " << services[i].name 
    << (services[i].is_activated ? " (ACT:Yes, " : " (ACT:No, ") 
    << (services[i].is_enabled ? " ENA:Yes)" : " ENA:No)");
    std::cout << std::endl;
  }
}

void create_service(ovpn_mana_handle_t handle, const char *name, const  char* subnet, int port)
{
  if (port <= 0 || port > 65535)
  {
    std::cerr << "Invalid port number. Use a number between 1 and 65535" << std::endl;
    return;
  }
  ovpn_err_t err = ovpn_mana_create_service(handle, name, subnet, port);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to create OpenVPN service" << std::endl;
    return;
  }
  std::cout << "OpenVPN service created successfully" << std::endl;
}

void delete_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_delete_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to delete OpenVPN service" << std::endl;
    return;
  }
  std::cout << "OpenVPN service deleted successfully" << std::endl;
}

void start_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_start_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to start OpenVPN service" << std::endl;
    return;
  }
  std::cout << "OpenVPN service started successfully" << std::endl;
}

void stop_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_stop_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to stop OpenVPN service" << std::endl;
    return;
  }
  std::cout << "OpenVPN service stopped successfully" << std::endl;
}

void restart_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_restart_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to restart OpenVPN service" << std::endl;
    return;
  }
  std::cout << "OpenVPN service restarted successfully" << std::endl;
}

/** 客户端相关命令处理函数 */

void create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip)
{
  ovpn_err_t err = ovpn_mana_create_client(handle, service_name, name, wanip);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to create OpenVPN client" << std::endl;
    return;
  }
  std::cout << "OpenVPN client created successfully" << std::endl;
}

void revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name)
{
  ovpn_err_t err = ovpn_mana_revoke_client(handle, service_name, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to revoke OpenVPN client" << std::endl;
    return;
  }
  std::cout << "OpenVPN client revoked successfully" << std::endl;
}

void list_online_clients(ovpn_mana_handle_t handle, const char *service_name)
{
  ovpn_client_t clients[10];
  int client_count = 0;
  ovpn_err_t err = ovpn_mana_get_online_clients(handle, service_name, clients, client_count);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to get online OpenVPN clients" << std::endl;
    return;
  }
  std::cout << "Online OpenVPN Clients:" << std::endl;
  for (int i = 0; i < client_count; ++i)
  {
    std::cout 
    << "  Name: " << clients[i].name 
    << ", Private IP: " << clients[i].private_ipv4 
    << ", Public IP: " << clients[i].public_ipv4 
    << ", Since: " << clients[i].since 
    << ", Bytes Received: " << clients[i].bytes_received 
    << ", Bytes Sent: " << clients[i].bytes_sent;
    std::cout << std::endl;
  }
}


int main(int argc, char *argv[])
{
  // 设置locale为中文
  setlocale(LC_ALL, "zh_CN.UTF-8");

  /// 打印一个title
  std::cout << "=====================================================================================" << std::endl;
  std::cout << "\t\tOpenVPN Command Line Manager" << std::endl;
  std::cout << "=====================================================================================" << std::endl;

  bool is_root = getuid() == 0;

  // 创建OpenVPN管理器实例
  ovpn_mana_handle_t handle = ovpn_mana_create();
  if (!handle)
  {
    std::cerr << "Failed to create OpenVPN manager" << std::endl;
    return -1;
  }

  if(!is_root) {

    std::cerr << "\033[31mPlease run this command line program with ROOT privileges\033[0m" << std::endl;
    return -1;
  }

  // 检查命令行参数
  if (argc < 2)
  {
    
    std::cerr << "Usage: " << argv[0] << " <command> [options]" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  service -l                                  List OpenVPN services" << std::endl;
    std::cerr << "  service -c <name>,<port>,<subnet>          Create OpenVPN service" << std::endl;
    std::cerr << "  service -d <name>                           Delete OpenVPN service" << std::endl;
    std::cerr << "  service -start <name>                       Start OpenVPN service" << std::endl;
    std::cerr << "  service -stop <name>                        Stop OpenVPN service" << std::endl;
    std::cerr << "  service -restart <name>                     Restart OpenVPN service" << std::endl;
    std::cerr << "  client  -c <service_name>,<name>,<wanip>    Create OpenVPN client" << std::endl;
    std::cerr << "  client  -d <service_name>,<name>            Revoke OpenVPN client" << std::endl;
    std::cerr << "  client  -l <service_name>                   List online OpenVPN clients" << std::endl;
    return -1;
  }
  
  // 解析命令行参数
  std::string command = argv[1];

  // 判断是否为service命令
  if(command == "service")
  {
    if (argc < 3)
    {
      std::cerr << "Usage: " << argv[0] << " service <command> [options]" << std::endl;
      return -1;
    }
    std::string sub_command = argv[2];
    if (sub_command == "-l")
    {
      print_services(handle);
      ovpn_mana_destroy(handle);
      return 0;
    }
    else if (sub_command == "-c")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " service -c <name>,<port>,<subnet>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      std::string service_info = argv[3];
      
      // 解析服务信息 service -c <name>,<port>,<subnet>
      size_t comma_pos1 = service_info.find(',');
      size_t comma_pos2 = service_info.find(',', comma_pos1 + 1);
      if (comma_pos1 == std::string::npos || comma_pos2 == std::string::npos)
      {
        std::cerr << "Invalid service info format. Use <name>,<port>,<subnet>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      std::string name = service_info.substr(0, comma_pos1);
      std::string port_str = service_info.substr(comma_pos1 + 1, comma_pos2 - comma_pos1 - 1);
      std::string subnet = service_info.substr(comma_pos2 + 1);
      int port = std::stoi(port_str);


      create_service(handle, name.c_str(), subnet.c_str(), port);
      ovpn_mana_destroy(handle);
      return 0;
    }
    else if (sub_command == "-d")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " service -d <name>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      const char *name = argv[3];
      delete_service(handle, name);
      ovpn_mana_destroy(handle);
      return 0;
    }
    else if(sub_command == "-start")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " service -start <name>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      const char *name = argv[3];
      start_service(handle, name);
      ovpn_mana_destroy(handle);
      return 0;
    }
    else if (sub_command == "-stop")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " service -stop <name>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      const char *name = argv[3];
      stop_service(handle, name);
      ovpn_mana_destroy(handle);
      return 0;
    }
    else if (sub_command == "-restart")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " service -restart <name>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      const char *name = argv[3];
      restart_service(handle, name);
      ovpn_mana_destroy(handle);
      return 0;
    }
  }

  // 如果是client命令
  if(command == "client")
  {
    if (argc < 3)
    {
      std::cerr << "Usage: " << argv[0] << " client <command> [options]" << std::endl;
      return -1;
    }
    std::string sub_command = argv[2];
    if (sub_command == "-c")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " client -c <service_name>,<name>,<wanip>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      std::string client_info = argv[3];
      
      // 解析服务信息 client -c <service_name>,<name>,<wanip>
      size_t comma_pos1 = client_info.find(',');
      size_t comma_pos2 = client_info.find(',', comma_pos1 + 1);
      if (comma_pos1 == std::string::npos || comma_pos2 == std::string::npos)
      {
        std::cerr << "Invalid client info format. Use <service_name>,<name>,<wanip>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      std::string service_name = client_info.substr(0, comma_pos1);
      std::string name = client_info.substr(comma_pos1 + 1, comma_pos2 - comma_pos1 - 1);
      std::string wanip = client_info.substr(comma_pos2 + 1);

      create_client(handle, service_name.c_str(), name.c_str(), wanip.c_str());
      ovpn_mana_destroy(handle);
      return 0;
    }
    else if (sub_command == "-d")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " client -d <service_name>,<name>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      std::string client_info = argv[3];
      size_t comma_pos = client_info.find(',');
      if (comma_pos == std::string::npos)
      {
        std::cerr << "Invalid client info format. Use <service_name>,<name>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      std::string service_name = client_info.substr(0, comma_pos);
      std::string name = client_info.substr(comma_pos + 1);
      revoke_client(handle, service_name.c_str(), name.c_str());
      ovpn_mana_destroy(handle);
      return 0;
    }
    else if (sub_command == "-l")
    {
      if (argc < 4)
      {
        std::cerr << "Usage: " << argv[0] << " client -l <service_name>" << std::endl;
        ovpn_mana_destroy(handle);
        return -1;
      }
      const char *service_name = argv[3];
      list_online_clients(handle, service_name);
      ovpn_mana_destroy(handle);
      return 0;
    }
  }
  // 销毁OpenVPN管理器实例
  ovpn_mana_destroy(handle);
  std::cout << "OpenVPN manager destroyed successfully" << std::endl;

  return 0;
}
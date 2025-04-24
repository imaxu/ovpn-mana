#include <iomanip>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include "ovpn-mana.hpp"

int main()
{
  /// 打印一个title
  std::cout << "=================" << std::endl;
  std::cout << "OpenVPN Manager" << std::endl;
  std::cout << "=================" << std::endl;

  // 创建OpenVPN管理器实例
  ovpn_mana_handle_t handle = ovpn_mana_create();
  if (!handle)
  {
    std::cerr << "Failed to create OpenVPN manager" << std::endl;
    return -1;
  }

  // 获取服务列表
  ovpn_service_t services[10];
  int service_count = 0;
  ovpn_err_t err = ovpn_mana_list_services(handle, services, service_count);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << "Failed to list OpenVPN services" << std::endl;
    ovpn_mana_destroy(handle);
    return -1;
  }
  std::cout << "OpenVPN Services:" << std::endl;

  std::cout << "0. 创建新服务" << std::endl;
  for (int i = 0; i < service_count; ++i)
  {
    std::cout << (i + 1) << "." << services[i].name << std::endl;
  }

  // 选择一个服务或者创建新的服务：(0-10)
  int choice;
  std::cout << "选择一个服务或者创建新的服务 (0-" << service_count << "): ";
  std::cin >> choice;
  if (choice < 0 || choice > service_count)
  {
    std::cerr << "Invalid choice" << std::endl;
    ovpn_mana_destroy(handle);
    return -1;
  }
  if (choice == 0)
  {
    // 交互创建服务
    std::string service_name;
    std::cout << "输入服务名（支持英文和数字）: ";
    std::cin >> service_name;
    int port;
    std::cout << "输入服务端口 (default 1194): ";
    std::cin >> port;
    if (port <= 0)
    {
      port = 1194; // 默认端口
    }
    std::cout << "开始创建服务.." << std::endl;
    err = ovpn_mana_create_service(handle, service_name.c_str(), port);
    if (err != OVPN_ERR_SUCCESS)
    {
      std::cerr << "Failed to create OpenVPN service" << std::endl;
      ovpn_mana_destroy(handle);
      return -1;
    }
    return 0;
  }

  // 选择服务
  std::string service_name = services[choice - 1].name;
  std::cout << "选择的服务: " << service_name << std::endl;
  // 是否创建客户端
  std::string create_client;
  std::cout << "是否创建客户端？(y/n): ";
  std::cin >> create_client;
  if (create_client == "y" || create_client == "Y")
  {
    std::string client_name;
    std::cout << "输入客户端名（支持英文和数字）: ";
    std::cin >> client_name;
    err = ovpn_mana_create_client(handle, service_name.c_str(), client_name.c_str());
    if (err != OVPN_ERR_SUCCESS)
    {
      std::cerr << "Failed to create OpenVPN client" << std::endl;
      ovpn_mana_destroy(handle);
      return -1;
    }
    // 获取客户端配置文件
    char config_content[2048] = {0};
    int config_content_size = 0;
    err = ovpn_mana_get_client_config(handle, service_name.c_str(), client_name.c_str(), config_content, config_content_size);
    if (config_content_size == 0)
    {
      std::cerr << "Failed to get OpenVPN client config file" << std::endl;
      ovpn_mana_destroy(handle);
      return -1;
    }
    std::cout << "=================================================" << std::endl;
    std::cout << "客户端配置文件内容:" << std::endl;
    std::cout << config_content << std::endl;
    std::cout << "请将以上内容保存为 " << client_name << ".ovpn" << std::endl;
    std::cout << "=================================================" << std::endl;
  }

  return 0;
}
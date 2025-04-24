#include <iomanip>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include "OpenVPNManager.hpp"

int main()
{
  // 创建服务
  if (OpenVPNManager::createService("myvpn", 1294))
  {
    std::cout << "服务创建成功" << std::endl;
  }

  // 启动服务
  if (OpenVPNManager::startService("myvpn"))
  {
    std::cout << "服务启动成功" << std::endl;
  }

  // 创建客户端
  if (OpenVPNManager::createClient("test_client", "myvpn"))
  {
    std::cout << "客户端创建成功" << std::endl;
  }

  // 获取在线客户端
  auto clients = OpenVPNManager::getOnlineClients("myvpn");
  for (const auto &client : clients)
  {
    std::cout << client.name << " (" << client.vpnIp << ") - "
              << "上传: " << client.bytesSent << " 下载: " << client.bytesReceived
              << std::endl;
  }

  // // 删除服务
  // if (OpenVPNManager::deleteService("oldvpn")) {
  //   std::cout << "服务删除成功" << std::endl;
  // }

  return 0;
}
#include "OpenVPNManager.hpp"
#include "config.hpp"
#include <iomanip>
#include <fstream>
#include <sstream>
#include <array>
#include <thread>
#include <iostream>
#include <memory>
#include <algorithm>

namespace fs = std::filesystem;

// 辅助函数：执行shell命令
bool OpenVPNManager::execCommand(const std::string &cmd, std::string &output)
{
  std::array<char, 128> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe)
    return false;
  while (fgets(buffer.data(), buffer.size(), pipe.get()))
  {
    output += buffer.data();
  }
  return true;
}

// 服务列表
std::vector<VPNService> OpenVPNManager::listServices()
{
  std::vector<VPNService> services;
  fs::path ovpnDir(OVPN_DIR);

  if (!fs::exists(ovpnDir))
    return services;

  for (const auto &entry : fs::directory_iterator(ovpnDir))
  {
    if (entry.is_regular_file() &&
        entry.path().extension() == ".conf" &&
        entry.path().filename().string().find("-server") != std::string::npos)
    {

      std::string name = entry.path().stem().string();
      name = name.substr(0, name.find("-server"));

      VPNService service;
      service.name = name;
      service.configPath = entry.path().string();
      service.isActive = isServiceActive(name);
      service.isEnabled = isServiceEnabled(name);
      services.push_back(service);
    }
  }
  return services;
}

// 创建服务
bool OpenVPNManager::createService(const std::string &name, int port)
{
  // 1. 生成服务器证书
  std::string cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa build-server-full " + name + "-server nopass";
  std::string output;
  if (!execCommand(cmd, output))
    return false;

  // 2. 拷贝证书文件
  auto copyFile = [](const std::string &src, const std::string &dest)
  {
    fs::create_directories(fs::path(dest).parent_path());
    fs::copy(src, dest, fs::copy_options::overwrite_existing);
  };

  std::string prefix = EASY_RSA_DIR + "/pki/";
  copyFile(prefix + "ca.crt", OVPN_DIR + "/" + name + "-ca.crt");
  copyFile(prefix + "issued/" + name + "-server.crt", OVPN_DIR + "/" + name + "-server.crt");
  copyFile(prefix + "private/" + name + "-server.key", OVPN_DIR + "/" + name + "-server.key");
  copyFile(prefix + "dh.pem", OVPN_DIR + "/" + name + "-dh.pem");

  // 生成TLS密钥
  cmd = OPENVPN_BIN + " --genkey secret " + OVPN_DIR + "/" + name + "-ta.key";
  if (!execCommand(cmd, output))
    return false;

  // 3. 生成配置文件
  std::ostringstream config;
  config << "port " << port << "\n"
         << "proto udp\n"
         << "dev tun\n"
         << "ca " << name << "-ca.crt\n"
         << "cert " << name << "-server.crt\n"
         << "key " << name << "-server.key\n"
         << "dh " << name << "-dh.pem\n"
         << "tls-auth " << name << "-ta.key 0\n"
         << "server 10.8.0.0 255.255.255.0\n"
         << "keepalive 10 120\n"
         << "persist-key\n"
         << "persist-tun\n"
         << "status " << name << "-status.log\n"
         << "verb 3\n";

  std::ofstream confFile(OVPN_DIR + "/" + name + "-server.conf");
  confFile << config.str();
  confFile.close();

  // 4. 启动并启用服务
  cmd = SYSTEMCTL_BIN + " start openvpn@" + name + "-server";
  if (!execCommand(cmd, output))
    return false;

  cmd = SYSTEMCTL_BIN + " enable openvpn@" + name + "-server";
  return execCommand(cmd, output);
}

// 检查服务是否活跃
bool OpenVPNManager::isServiceActive(const std::string &name)
{
  std::string cmd = SYSTEMCTL_BIN + " is-active openvpn@" + name + "-server";
  std::string output;
  return execCommand(cmd, output) && output.find("active") != std::string::npos;
}

// 检查服务是否启用
bool OpenVPNManager::isServiceEnabled(const std::string &name)
{
  std::string cmd = SYSTEMCTL_BIN + " is-enabled openvpn@" + name + "-server";
  std::string output;
  return execCommand(cmd, output) && output.find("enabled") != std::string::npos;
}

// 启动服务
bool OpenVPNManager::startService(const std::string &name)
{
  std::string cmd = SYSTEMCTL_BIN + " start openvpn@" + name + "-server";
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to start service: " << output << std::endl;
    return false;
  }
  return true;
}

// 停止服务
bool OpenVPNManager::stopService(const std::string &name)
{
  std::string cmd = SYSTEMCTL_BIN + " stop openvpn@" + name + "-server";
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to stop service: " << output << std::endl;
    return false;
  }

  // 确保服务已停止
  int attempts = 0;
  while (isServiceActive(name) && attempts++ < 5)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return !isServiceActive(name);
}

// 重启服务
bool OpenVPNManager::restartService(const std::string &name)
{
  std::string cmd = SYSTEMCTL_BIN + " restart openvpn@" + name + "-server";
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to restart service: " << output << std::endl;
    return false;
  }
  return true;
}

// 删除服务
bool OpenVPNManager::deleteService(const std::string &name)
{
  // 1. 停止服务
  if (isServiceActive(name))
  {
    if (!stopService(name))
    {
      std::cerr << "Failed to stop service before deletion" << std::endl;
      return false;
    }
  }

  // 2. 禁用服务
  std::string cmd = SYSTEMCTL_BIN + " disable openvpn@" + name + "-server";
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to disable service: " << output << std::endl;
    return false;
  }

  // 3. 删除配置文件
  std::vector<std::string> filesToDelete = {
      OVPN_DIR + "/" + name + "-server.conf",
      OVPN_DIR + "/" + name + "-ca.crt",
      OVPN_DIR + "/" + name + "-server.crt",
      OVPN_DIR + "/" + name + "-server.key",
      OVPN_DIR + "/" + name + "-dh.pem",
      OVPN_DIR + "/" + name + "-ta.key",
      OVPN_DIR + "/" + name + "-status.log"};

  bool success = true;
  for (const auto &file : filesToDelete)
  {
    if (fs::exists(file))
    {
      try
      {
        fs::remove(file);
      }
      catch (const fs::filesystem_error &e)
      {
        std::cerr << "Failed to delete file " << file << ": " << e.what() << std::endl;
        success = false;
      }
    }
  }

  // 4. 从easy-rsa吊销服务器证书
  cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa --batch revoke " + name + "-server";
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to revoke server certificate: " << output << std::endl;
    success = false;
  }

  // 5. 生成新的CRL
  cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa gen-crl";
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to generate new CRL: " << output << std::endl;
    success = false;
  }

  // 6. 更新OpenVPN的CRL文件
  if (fs::exists(EASY_RSA_DIR + "/pki/crl.pem"))
  {
    try
    {
      fs::copy(EASY_RSA_DIR + "/pki/crl.pem",
               OVPN_DIR + "/crl.pem",
               fs::copy_options::overwrite_existing);
    }
    catch (const fs::filesystem_error &e)
    {
      std::cerr << "Failed to update CRL file: " << e.what() << std::endl;
      success = false;
    }
  }

  return success;
}

// 客户端管理
bool OpenVPNManager::createClient(const std::string &name, const std::string &serviceName)
{
  std::string cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa build-client-full " + name + " nopass";
  std::string output;
  if (!execCommand(cmd, output))
    return false;

  // 生成客户端配置文件
  std::ostringstream config;
  config << "client\n"
         << "dev tun\n"
         << "proto udp\n"
         << "remote YOUR_SERVER_IP 1194\n"
         << "resolv-retry infinite\n"
         << "nobind\n"
         << "persist-key\n"
         << "persist-tun\n"
         << "remote-cert-tls server\n"
         << "cipher AES-256-CBC\n"
         << "verb 3\n";

  // 添加证书内容
  auto addSection = [&](const std::string &file, const std::string &tag)
  {
    if (fs::exists(file))
    {
      config << "<" << tag << ">\n";
      std::ifstream f(file);
      config << f.rdbuf();
      config << "</" << tag << ">\n";
    }
  };

  addSection(EASY_RSA_DIR + "/pki/ca.crt", "ca");
  addSection(EASY_RSA_DIR + "/pki/issued/" + name + ".crt", "cert");
  addSection(EASY_RSA_DIR + "/pki/private/" + name + ".key", "key");
  addSection(OVPN_DIR + "/" + serviceName + "-ta.key", "tls-auth");
  config << "key-direction 1\n";

  // 写入文件
  fs::path configPath = getClientConfigPath(name, serviceName);
  fs::create_directories(configPath.parent_path());
  std::ofstream out(configPath);
  out << config.str();

  return true;
}
// 吊销客户端
bool OpenVPNManager::revokeClient(const std::string &name, const std::string &serviceName)
{
  // 1. 吊销证书
  std::string cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa --batch revoke " + name;
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to revoke client certificate: " << output << std::endl;
    return false;
  }

  // 2. 生成新的CRL
  cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa gen-crl";
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to generate CRL: " << output << std::endl;
    return false;
  }

  // 3. 更新OpenVPN的CRL文件
  if (fs::exists(EASY_RSA_DIR + "/pki/crl.pem"))
  {
    try
    {
      fs::copy(EASY_RSA_DIR + "/pki/crl.pem",
               OVPN_DIR + "/crl.pem",
               fs::copy_options::overwrite_existing);
    }
    catch (const fs::filesystem_error &e)
    {
      std::cerr << "Failed to update CRL file: " << e.what() << std::endl;
      return false;
    }
  }

  // 4. 重载OpenVPN服务
  if (!restartService(serviceName))
  {
    std::cerr << "Failed to restart OpenVPN service" << std::endl;
    return false;
  }

  // 5. 删除客户端文件
  std::vector<std::string> filesToDelete = {
      EASY_RSA_DIR + "/pki/issued/" + name + ".crt",
      EASY_RSA_DIR + "/pki/private/" + name + ".key",
      EASY_RSA_DIR + "/pki/reqs/" + name + ".req",
      getClientConfigPath(name, serviceName)};

  bool success = true;
  for (const auto &file : filesToDelete)
  {
    if (fs::exists(file))
    {
      try
      {
        fs::remove(file);
      }
      catch (const fs::filesystem_error &e)
      {
        std::cerr << "Failed to delete file " << file << ": " << e.what() << std::endl;
        success = false;
      }
    }
  }

  return success;
}

// 获取在线客户端列表
std::vector<VPNClient> OpenVPNManager::getOnlineClients(const std::string &serviceName)
{
  std::vector<VPNClient> clients;
  std::string statusFile = OVPN_DIR + "/" + serviceName + "-status.log";

  if (!fs::exists(statusFile))
  {
    std::cerr << "Status file not found: " << statusFile << std::endl;
    return clients;
  }

  std::ifstream file(statusFile);
  if (!file.is_open())
  {
    std::cerr << "Failed to open status file: " << statusFile << std::endl;
    return clients;
  }

  std::string line;
  bool inClientSection = false;
  bool inRoutingSection = false;
  std::map<std::string, VPNClient> clientMap;

  while (std::getline(file, line))
  {
    // 清理行尾
    line.erase(std::remove_if(line.begin(), line.end(),
                              [](char c)
                              { return c == '\r' || c == '\n'; }),
               line.end());

    if (line.empty())
      continue;

    if (line.find("CLIENT_LIST") == 0)
    {
      inClientSection = true;
      inRoutingSection = false;
      continue;
    }
    else if (line.find("ROUTING_TABLE") == 0)
    {
      inClientSection = false;
      inRoutingSection = true;
      continue;
    }
    else if (line.find("GLOBAL_STATS") == 0)
    {
      break;
    }

    if (inClientSection)
    {
      std::vector<std::string> tokens;
      std::istringstream iss(line);
      std::string token;

      while (std::getline(iss, token, ','))
      {
        tokens.push_back(token);
      }

      if (tokens.size() >= 6)
      {
        VPNClient client;
        client.name = tokens[0];
        client.realIp = tokens[1];
        try
        {
          client.bytesReceived = std::stoull(tokens[2]);
          client.bytesSent = std::stoull(tokens[3]);
        }
        catch (...)
        {
          client.bytesReceived = 0;
          client.bytesSent = 0;
        }
        client.since = tokens[4];
        clientMap[client.name] = client;
      }
    }
    else if (inRoutingSection)
    {
      std::vector<std::string> tokens;
      std::istringstream iss(line);
      std::string token;

      while (std::getline(iss, token, ','))
      {
        tokens.push_back(token);
      }

      if (tokens.size() >= 3)
      {
        std::string vpnIp = tokens[0];
        std::string name = tokens[1];

        if (clientMap.count(name))
        {
          clientMap[name].vpnIp = vpnIp;
        }
      }
    }
  }

  // 转换为vector并排序
  for (auto &pair : clientMap)
  {
    clients.push_back(pair.second);
  }

  std::sort(clients.begin(), clients.end(),
            [](const VPNClient &a, const VPNClient &b)
            {
              return a.since < b.since;
            });

  return clients;
}

// 获取客户端配置文件路径
std::string OpenVPNManager::getClientConfigPath(const std::string &name, const std::string &serviceName)
{
  return OVPN_DIR + "/client-configs/" + serviceName + "/" + name + ".ovpn";
}
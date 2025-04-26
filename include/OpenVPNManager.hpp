#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

struct VPNService
{
  std::string name;
  std::string configPath;
  bool isActive;
  bool isEnabled;
};

struct VPNClient
{
  std::string name;
  std::string vpnIp;
  std::string realIp;
  std::string since;
  uint64_t bytesReceived;
  uint64_t bytesSent;
};

class OpenVPNManager
{
public:
  // 服务管理
  static std::vector<VPNService> listServices();
  static bool createService(const std::string &name, const std::string &subnet, int port = 1194);
  static bool startService(const std::string &name);
  static bool stopService(const std::string &name);
  static bool restartService(const std::string &name);
  static bool deleteService(const std::string &name);

  // 客户端管理
  static bool createClient(const std::string &name, const std::string &serviceName, const std::string &wanip);
  static bool revokeClient(const std::string &name, const std::string &serviceName);
  static std::vector<VPNClient> getOnlineClients(const std::string &serviceName);
  static std::string getOVPNFileContent(const std::string &name, const std::string &serviceName);

private:
  static bool execCommand(const std::string &cmd, std::string &output);
  static bool isServiceActive(const std::string &name);
  static bool isServiceEnabled(const std::string &name);
  static std::string getServiceConfigPath(const std::string &name);
  static std::string getClientConfigPath(const std::string &name, const std::string &serviceName);
  static std::string getStatusFilePath(const std::string &serviceName);
  static bool copyWithSudo(const std::string &src, const std::string &dest)
  {
    try
    {
      // 检查源文件是否存在
      if (!fs::exists(src))
      {
        throw std::runtime_error("Source file does not exist");
      }

      // 使用临时文件避免权限问题
      fs::path tempDest = fs::path(dest).parent_path() / ("temp_" + fs::path(dest).filename().string());

      // 先复制到临时文件
      fs::copy(src, tempDest, fs::copy_options::overwrite_existing);

      // 使用sudo移动文件
      std::string cmd = "sudo mv " + tempDest.string() + " " + dest;
      std::string output;
      if (!execCommand(cmd, output))
      {
        fs::remove(tempDest); // 清理临时文件
        throw std::runtime_error("Failed to move file with sudo: " + output);
      }

      // 设置权限
      cmd = "sudo chmod 644 " + dest;
      if (!execCommand(cmd, output))
      {
        throw std::runtime_error("Failed to set permissions: " + output);
      }

      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "File operation failed: " << e.what() << std::endl;
      return false;
    }
  }
};
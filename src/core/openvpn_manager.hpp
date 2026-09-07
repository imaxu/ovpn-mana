#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <iostream>
#include "app_config.hpp"

namespace fs = std::filesystem;

struct VPNService {
    std::string name;
    std::string configPath;
    int port;
    std::string subnet;
    bool isActive;
    bool isEnabled;
};

struct VPNClient {
    std::string name;
    std::string vpnIp;
    std::string realIp;
    std::string since;
    uint64_t bytesReceived;
    uint64_t bytesSent;
};

class OpenVPNManager {
public:
    AppConfig m_config{AppConfig::defaults()};

    void configure(const AppConfig& cfg) { m_config = cfg; }
    const AppConfig& config() const { return m_config; }

    std::vector<VPNService> listServices();
    bool createService(const std::string &name, const std::string &subnet, int port = 1194);
    bool startService(const std::string &name);
    bool stopService(const std::string &name);
    bool restartService(const std::string &name);
    bool deleteService(const std::string &name);

    bool createClient(const std::string &name, const std::string &serviceName, const std::string &wanip, const std::string &client_ip = "");
    bool revokeClient(const std::string &name, const std::string &serviceName);
    std::vector<VPNClient> getOnlineClients(const std::string &serviceName);
    int getTotalClientsCount(const std::string &serviceName);
    std::vector<VPNClient> getTotalClients(const std::string &serviceName);
    std::string getOVPNFileContent(const std::string &name, const std::string &serviceName);

private:
    bool execCommand(const std::string &cmd, std::string &output);
    bool isServiceActive(const std::string &name);
    bool isServiceEnabled(const std::string &name);
    std::string getServiceConfigPath(const std::string &name);
    std::string getClientConfigPath(const std::string &name, const std::string &serviceName);
    std::string getStatusFilePath(const std::string &serviceName);
    bool copyWithSudo(const std::string &src, const std::string &dest)
    {
        try
        {
            if (!fs::exists(src))
            {
                throw std::runtime_error("Source file does not exist");
            }

            fs::path tempDest = fs::path(dest).parent_path() / ("temp_" + fs::path(dest).filename().string());
            fs::copy(src, tempDest, fs::copy_options::overwrite_existing);

            std::string cmd = "sudo mv " + tempDest.string() + " " + dest;
            std::string output;
            if (!execCommand(cmd, output))
            {
                fs::remove(tempDest);
                throw std::runtime_error("Failed to move file with sudo: " + output);
            }

            cmd = "sudo chmod 600 " + dest;
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
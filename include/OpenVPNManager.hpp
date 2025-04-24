#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

struct VPNService {
    std::string name;
    std::string configPath;
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
    // 服务管理
    static std::vector<VPNService> listServices();
    static bool createService(const std::string& name, int port = 1194);
    static bool startService(const std::string& name);
    static bool stopService(const std::string& name);
    static bool restartService(const std::string& name);
    static bool deleteService(const std::string& name);

    // 客户端管理
    static bool createClient(const std::string& name, const std::string& serviceName);
    static bool revokeClient(const std::string& name, const std::string& serviceName);
    static std::vector<VPNClient> getOnlineClients(const std::string& serviceName);

private:
    static bool execCommand(const std::string& cmd, std::string& output);
    static bool isServiceActive(const std::string& name);
    static bool isServiceEnabled(const std::string& name);
    static std::string getServiceConfigPath(const std::string& name);
    static std::string getClientConfigPath(const std::string& name, const std::string& serviceName);
    static std::string getStatusFilePath(const std::string& serviceName);
};
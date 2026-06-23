#include "core/openvpn_manager.hpp"
#include "core/command_templates.hpp"
#include "core/validators.hpp"
#include "ovpn-mana/ovpn_mana_platform.h"
#include <iomanip>
#include <fstream>
#include <sstream>
#include <array>
#include <thread>
#include <iostream>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

bool OpenVPNManager::execCommand(const std::string &cmd, std::string &output)
{
  std::cout << "Executing command: " << cmd << std::endl;

  std::string fullCmd = cmd + " 2>&1";
  std::array<char, 128> buffer;
  FILE *pipe = OVPN_POPEN(fullCmd.c_str(), "r");
  if (!pipe) {
    std::cerr << "Failed to execute command" << std::endl;
    return false;
  }
  while (fgets(buffer.data(), buffer.size(), pipe))
    output += buffer.data();

  int status = OVPN_PCLOSE(pipe);
  bool success = (status == 0);

  if (!output.empty() && output.back() == '\n')
    output.pop_back();
  std::cout << "Executing command result: " << output << std::endl;

  if (!success)
    std::cerr << "Command failed with exit code: " << status << std::endl;

  return success;
}

std::vector<VPNService> OpenVPNManager::listServices()
{
  std::vector<VPNService> services;
  fs::path ovpnDir(m_config.ovpn_dir);

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
      service.port = 0;

      std::ifstream confFile(entry.path());
      if (confFile.is_open()) {
        std::string line;
        while (std::getline(confFile, line)) {
          if (line.rfind("port ", 0) == 0) {
            try { service.port = std::stoi(line.substr(5)); } catch (...) {}
          } else if (line.rfind("server ", 0) == 0) {
            std::string s = line.substr(7);
            size_t sp = s.find(' ');
            if (sp != std::string::npos)
              service.subnet = s.substr(0, sp);
          }
        }
      }

      service.isActive = isServiceActive(name);
      service.isEnabled = isServiceEnabled(name);
      services.push_back(service);
    }
  }
  return services;
}

bool OpenVPNManager::createService(const std::string &name, const std::string& subnet, int port)
{
  if (OVPN_GETUID() != 0)
  {
    std::cerr << "Error: This operation requires root privileges. Please use sudo." << std::endl;
    return false;
  }
  std::cout << "Checking permissions corrected." << std::endl;

  if (!fs::exists(m_config.easy_rsa_dir)) {
    try {
      fs::create_directories(m_config.easy_rsa_dir);
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Failed to create PKI workspace " << m_config.easy_rsa_dir << ": " << e.what() << std::endl;
      return false;
    }
  }

  if (!fs::exists(m_config.easy_rsa_dir + "/pki/ca.crt")) {
    std::cerr << "CA not found. Please initialize PKI first:" << std::endl;
    std::cerr << "  cd " << m_config.easy_rsa_dir << " && sudo easyrsa init-pki" << std::endl;
    std::cerr << "  sudo easyrsa build-ca nopass" << std::endl;
    return false;
  }

  std::string cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_GEN_REQ, m_config, {{"NAME", name + "-server nopass"}});
  std::string output;
  if (!execCommand(cmd, output))
    return false;
  std::cout << "Created new request for " << name << std::endl;

  cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_SIGN_REQ_SERVER, m_config, {{"NAME", name + "-server"}});
  if (!execCommand(cmd, output))
    return false;
  std::cout << "Signed request for " << name << std::endl;

  if (!fs::exists(m_config.easy_rsa_dir + "/pki/dh.pem"))
  {
    std::cerr << "dh.pem not found, generating..." << std::endl;
    cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_GEN_DH, m_config);
    if (!execCommand(cmd, output))
    {
      std::cerr << "Failed to generate dh.pem: " << output << std::endl;
      return false;
    }
  }

  fs::path ccdDir = fs::path(m_config.ovpn_server_conf_dir()) / name / "ccd";
  if (!fs::exists(ccdDir))
  {
    try
    {
      fs::create_directories(ccdDir);
    }
    catch (const fs::filesystem_error &e)
    {
      std::cerr << "Failed to create directory " << ccdDir << ": " << e.what() << std::endl;
      return false;
    }
  }

  auto copyWithSudo = [this](const std::string &src, const std::string &dest) -> bool
  {
    std::string cmd = ovpn::commands::replace(ovpn::commands::CP_WITH_SUDO, m_config, {{"SRC", src}, {"DEST", dest}});
    std::string output;
    if (!execCommand(cmd, output))
    {
      std::cerr << "Failed to copy " << src << " to " << dest
                << ": " << output << std::endl;
      return false;
    }

    cmd = ovpn::commands::replace(ovpn::commands::CHMOD, m_config, {{"MODE", "644"}, {"PATH", dest}});
    return execCommand(cmd, output);
  };

  fs::path prefix = fs::path(m_config.easy_rsa_dir) / "pki";
  if (!copyWithSudo((prefix / "ca.crt").string(), (fs::path(m_config.ovpn_server_conf_dir()) / name / "ca.crt").string()) ||
      !copyWithSudo((prefix / "issued" / (name + "-server.crt")).string(), (fs::path(m_config.ovpn_server_conf_dir()) / name / "server.crt").string()) ||
      !copyWithSudo((prefix / "private" / (name + "-server.key")).string(), (fs::path(m_config.ovpn_server_conf_dir()) / name / "server.key").string()) ||
      !copyWithSudo((prefix / "dh.pem").string(), (fs::path(m_config.ovpn_server_conf_dir()) / name / "dh.pem").string()))
  {
    return false;
  }

  cmd = ovpn::commands::replace(ovpn::commands::OPENVPN_GEN_TA_KEY, m_config, {{"OUTPUT_PATH", (fs::path(m_config.ovpn_server_conf_dir()) / name / "ta.key").string()}});
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to generate TLS key: " << output << std::endl;
    return false;
  }

  std::string serverDir = m_config.ovpn_server_conf_dir() + "/" + name;
  std::string configContent = ovpn::commands::replace(ovpn::commands::SERVER_CONFIG, m_config, {
      {"PORT", std::to_string(port)},
      {"SUBNET", subnet},
      {"SERVER_DIR", serverDir}
  });

  std::ofstream confFile(fs::path(m_config.ovpn_dir) / (name + "-server.conf"));
  confFile << configContent;
  confFile.close();

  std::string serverSuffix = name + "-server";
  cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_START, m_config, {{"NAME", serverSuffix}});
  if (!execCommand(cmd, output))
    return false;

  cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_ENABLE, m_config, {{"NAME", serverSuffix}});
  return execCommand(cmd, output);
}

bool OpenVPNManager::isServiceActive(const std::string &name)
{
  std::string cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_IS_ACTIVE, m_config, {{"NAME", name + "-server"}});
  std::string output;
  return execCommand(cmd, output) && output.find("inactive") == std::string::npos;
}

bool OpenVPNManager::isServiceEnabled(const std::string &name)
{
  std::string cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_IS_ENABLED, m_config, {{"NAME", name + "-server"}});
  std::string output;
  return execCommand(cmd, output) && output.find("enabled") != std::string::npos;
}

bool OpenVPNManager::startService(const std::string &name)
{
  std::string cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_START, m_config, {{"NAME", name + "-server"}});
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to start service: " << output << std::endl;
    return false;
  }
  return true;
}

bool OpenVPNManager::stopService(const std::string &name)
{
  std::string cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_STOP, m_config, {{"NAME", name + "-server"}});
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to stop service: " << output << std::endl;
    return false;
  }

  int attempts = 0;
  while (isServiceActive(name) && attempts++ < 5)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return !isServiceActive(name);
}

bool OpenVPNManager::restartService(const std::string &name)
{
  std::string cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_RESTART, m_config, {{"NAME", name + "-server"}});
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to restart service: " << output << std::endl;
    return false;
  }
  return true;
}

bool OpenVPNManager::deleteService(const std::string &name)
{
  const fs::path serverRootPath = fs::path(m_config.ovpn_server_conf_dir()) / name;
  if (isServiceActive(name))
  {
    if (!stopService(name))
    {
      std::cerr << "Failed to stop service before deletion" << std::endl;
      return false;
    }
  }

  std::string cmd = ovpn::commands::replace(ovpn::commands::SYSTEMCTL_DISABLE, m_config, {{"NAME", name + "-server"}});
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to disable service: " << output << std::endl;
    return false;
  }

  std::vector<std::string> filesToDelete = {
    (fs::path(m_config.ovpn_dir) / (name + "-server.conf")).string(),
    (serverRootPath / "ca.crt").string(),
    (serverRootPath / "server.crt").string(),
    (serverRootPath / "server.key").string(),
    (serverRootPath / "dh.pem").string(),
    (serverRootPath / "ta.key").string(),
    (serverRootPath / "ipp.txt").string(),
    (serverRootPath / "status.log").string()};

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

  try
  {
    fs::remove_all(fs::path(m_config.ovpn_server_conf_dir()) / name);
  }
  catch (const fs::filesystem_error &e)
  {
    std::cerr << "Failed to delete directory " << m_config.ovpn_server_conf_dir() + "/" + name << ": " << e.what() << std::endl;
    success = false;
  }

  bool caExists = fs::exists(m_config.easy_rsa_dir + "/pki/ca.crt");
  if (caExists) {
    cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_REVOKE, m_config, {{"NAME", name + "-server"}});
    if (!execCommand(cmd, output))
    {
      std::cerr << "Warning: Failed to revoke server certificate: " << output << std::endl;
    }

    cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_GEN_CRL, m_config);
    if (!execCommand(cmd, output))
    {
      std::cerr << "Warning: Failed to generate new CRL: " << output << std::endl;
    }

    if (fs::exists(m_config.easy_rsa_dir + "/pki/crl.pem"))
    {
      try
      {
        fs::copy(m_config.easy_rsa_dir + "/pki/crl.pem",
                 m_config.ovpn_dir + "/crl.pem",
                 fs::copy_options::overwrite_existing);
      }
      catch (const fs::filesystem_error &e)
      {
        std::cerr << "Warning: Failed to update CRL file: " << e.what() << std::endl;
      }
    }
  } else {
    std::cerr << "Warning: CA not found, skipping certificate revocation." << std::endl;
    std::cerr << "  Run 'cd " << m_config.easy_rsa_dir << " && sudo easyrsa init-pki" << std::endl;
    std::cerr << "  sudo easyrsa build-ca nopass' first." << std::endl;
  }

  return success;
}

bool OpenVPNManager::createClient(const std::string &name, const std::string &serviceName, const std::string &wanip, const std::string &client_ip)
{
  if (name.empty() || serviceName.empty() || wanip.empty())
  {
    std::cerr << "Error: client name, service name and WAN IP are required" << std::endl;
    return false;
  }

  std::string serviceConfigPath = getServiceConfigPath(serviceName);
  if (!fs::exists(serviceConfigPath))
  {
    std::cerr << "Service config file not found: " << serviceConfigPath << std::endl;
    return false;
  }
  std::ifstream configFile(serviceConfigPath);
  if (!configFile.is_open())
  {
    std::cerr << "Failed to open service config file: " << serviceConfigPath << std::endl;
    return false;
  }
  std::string configContent((std::istreambuf_iterator<char>(configFile)),
                             std::istreambuf_iterator<char>());
  configFile.close();

  std::string portStr = configContent.substr(configContent.find("port ") + 5);
  portStr = portStr.substr(0, portStr.find("\n"));
  int port = std::stoi(portStr);
  std::cout << "Service port: " << port << std::endl;

  std::string ccdDir = (fs::path(m_config.ovpn_server_conf_dir()) / serviceName / "ccd").string();
  if (!client_ip.empty())
  {
    auto ipResult = ovpn::validators::validateIPv4(client_ip);
    if (!ipResult.valid)
    {
      std::cerr << "Invalid client IP '" << client_ip << "': " << ipResult.reason << std::endl;
      return false;
    }

    fs::create_directories(ccdDir);
    for (const auto &entry : fs::directory_iterator(ccdDir))
    {
      if (!entry.is_regular_file()) continue;
      std::ifstream f(entry.path());
      std::string line;
      while (std::getline(f, line))
      {
        if (line.find("ifconfig-push") != std::string::npos && line.find(client_ip) != std::string::npos)
        {
          std::cerr << "IP conflict: " << client_ip << " is already assigned to another client" << std::endl;
          return false;
        }
      }
    }
  }

  std::string cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_BUILD_CLIENT_FULL, m_config, {{"NAME", name}});
  std::string output;
  if (!execCommand(cmd, output))
    return false;

  std::cout << "build-client-full done!" << std::endl;
  std::cout << "Ready to build client configuration." << std::endl;

  std::ostringstream config;
  config << ovpn::commands::replace(ovpn::commands::CLIENT_CONFIG, m_config, {
      {"WAN_IP", wanip},
      {"PORT", std::to_string(port)}
  });

  auto addSection = [&](const std::string &file, const std::string &tag)
  {
    std::cout << "Adding section: " << tag << " from file: " << file << std::endl;
    if (fs::exists(file))
    {
      config << "<" << tag << ">\n";
      std::ifstream f(file);
      config << f.rdbuf();
      config << "</" << tag << ">\n";
    }
  };

  addSection((fs::path(m_config.easy_rsa_dir) / "pki" / "ca.crt").string(), "ca");
  addSection((fs::path(m_config.easy_rsa_dir) / "pki" / "issued" / (name + ".crt")).string(), "cert");
  addSection((fs::path(m_config.easy_rsa_dir) / "pki" / "private" / (name + ".key")).string(), "key");
  addSection((fs::path(m_config.ovpn_server_conf_dir()) / serviceName / "ta.key").string(), "tls-auth");
  config << "key-direction 1\n";

  if (!client_ip.empty())
  {
    std::ofstream ccdFile(fs::path(ccdDir) / name);
    if (ccdFile.is_open())
    {
      ccdFile << "ifconfig-push " << client_ip << " 255.255.255.0\n";
      std::cout << "CCD fixed IP set: " << client_ip << " for client " << name << std::endl;
    }
    else
    {
      std::cerr << "Warning: failed to write CCD file for " << name << std::endl;
    }
  }

  fs::path configPath = getClientConfigPath(name, serviceName);
  std::cout << "Writing client config to: " << configPath << std::endl;
  fs::create_directories(configPath.parent_path());
  std::ofstream out(configPath);
  out << config.str();
  std::cout << "Writing client config done!" << std::endl;

  return true;
}

bool OpenVPNManager::revokeClient(const std::string &name, const std::string &serviceName)
{
  std::string cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_REVOKE, m_config, {{"NAME", name}});
  std::string output;
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to revoke client certificate: " << output << std::endl;
    return false;
  }

  cmd = ovpn::commands::replace(ovpn::commands::EASYRSA_GEN_CRL, m_config);
  if (!execCommand(cmd, output))
  {
    std::cerr << "Failed to generate CRL: " << output << std::endl;
    return false;
  }

  if (fs::exists(m_config.easy_rsa_dir + "/pki/crl.pem"))
  {
    try
    {
      fs::copy(m_config.easy_rsa_dir + "/pki/crl.pem",
               m_config.ovpn_dir + "/crl.pem",
               fs::copy_options::overwrite_existing);
    }
    catch (const fs::filesystem_error &e)
    {
      std::cerr << "Failed to update CRL file: " << e.what() << std::endl;
      return false;
    }
  }

  std::cout << "Client certificate revoked. CRL updated." << std::endl;
  std::cout << "To apply immediately with minimal disruption, run:" << std::endl;
  std::cout << "  sudo systemctl reload openvpn@" << serviceName << "-server" << std::endl;
  std::cout << "Or schedule a restart during maintenance window:" << std::endl;
  std::cout << "  sudo systemctl restart openvpn@" << serviceName << "-server" << std::endl;

  std::vector<std::string> filesToDelete = {
      m_config.easy_rsa_dir + "/pki/issued/" + name + ".crt",
      m_config.easy_rsa_dir + "/pki/private/" + name + ".key",
      m_config.easy_rsa_dir + "/pki/reqs/" + name + ".req",
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

std::string OpenVPNManager::getOVPNFileContent(const std::string &name, const std::string &serviceName)
{
  std::string configPath = getClientConfigPath(name, serviceName);
  if (!fs::exists(configPath))
  {
    std::cerr << "Config file not found: " << configPath << std::endl;
    return "";
  }

  std::ifstream file(configPath);
  if (!file.is_open())
  {
    std::cerr << "Failed to open config file: " << configPath << std::endl;
    return "";
  }

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  return content;
}

std::vector<VPNClient> OpenVPNManager::getOnlineClients(const std::string &serviceName)
{
  std::vector<VPNClient> clients;
  std::string statusFile = (fs::path(m_config.ovpn_server_conf_dir()) / serviceName / "status.log").string();

  std::ifstream file(statusFile);
  if (!file.is_open()) {
      std::cerr << "[ERROR] Cannot open status file: " << statusFile << std::endl;
      return clients;
  }

  std::string line;
  bool inClientSection = false;
  bool inRoutingSection = false;
  std::map<std::string, VPNClient> clientMap;

  while (std::getline(file, line)) {
      line.erase(std::remove_if(line.begin(), line.end(),
                 [](char c) { return c == '\r' || c == '\n'; }),
                 line.end());

      if (line.empty()) continue;

      if (line.find("OpenVPN CLIENT LIST") != std::string::npos) {
          inClientSection = true;
          inRoutingSection = false;
          continue;
      } else if (line.find("ROUTING TABLE") != std::string::npos) {
          inClientSection = false;
          inRoutingSection = true;
          continue;
      } else if (line.find("GLOBAL STATS") != std::string::npos) {
          break;
      }

      if (inClientSection) {
          if (line.find("Common Name") != std::string::npos) continue;

          std::istringstream iss(line);
          std::vector<std::string> tokens;
          std::string token;

          while (std::getline(iss, token, ',')) {
              tokens.push_back(token);
          }

          if (tokens.size() >= 5) {
              VPNClient client;
              client.name = tokens[0];
              client.realIp = tokens[1];
              try {
                  client.bytesReceived = std::stoull(tokens[2]);
                  client.bytesSent = std::stoull(tokens[3]);
              } catch (...) {
                  client.bytesReceived = 0;
                  client.bytesSent = 0;
              }
              client.since = tokens[4];
              clientMap[client.name] = client;
          }
      }
      else if (inRoutingSection) {
          if (line.find("Virtual Address") != std::string::npos) continue;

          std::istringstream iss(line);
          std::vector<std::string> tokens;
          std::string token;

          while (std::getline(iss, token, ',')) {
              tokens.push_back(token);
          }

          if (tokens.size() >= 2) {
              std::string vpnIp = tokens[0];
              std::string name = tokens[1];

              if (clientMap.count(name)) {
                  clientMap[name].vpnIp = vpnIp;
              }
          }
      }
  }

  for (const auto &pair : clientMap) {
      clients.push_back(pair.second);
  }

  std::sort(clients.begin(), clients.end(),
            [](const VPNClient &a, const VPNClient &b) {
                return a.since < b.since;
            });

  return clients;
}

int OpenVPNManager::getTotalClientsCount(const std::string &serviceName)
{
  fs::path clientConfigDir = fs::path(m_config.ovpn_dir) / "client-configs" / serviceName;

  if (!fs::exists(clientConfigDir) || !fs::is_directory(clientConfigDir)) {
      return 0;
  }

  int count = 0;
  for (const auto &entry : fs::directory_iterator(clientConfigDir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".ovpn") {
          count++;
      }
  }

  return count;
}

std::string OpenVPNManager::getClientConfigPath(const std::string &name, const std::string &serviceName)
{
  return (fs::path(m_config.ovpn_dir) / "client-configs" / serviceName / (name + ".ovpn")).string();
}

std::string OpenVPNManager::getServiceConfigPath(const std::string &name)
{
  return (fs::path(m_config.ovpn_dir) / (name + "-server.conf")).string();
}
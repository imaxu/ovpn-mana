#include "cli/cli_handler.hpp"
#include "cli/console_renderer.hpp"
#include "ovpn-mana/ovpn_mana_version.h"
#include "ovpn-mana/ovpn_mana_platform.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

namespace ovpn::cli {

static void print_services(ovpn_mana_handle_t handle)
{
  ovpn_service_t services[10];
  int service_count = 0;
  ovpn_err_t err = ovpn_mana_list_services(handle, services, service_count);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to list OpenVPN services" << color::RESET << std::endl;
    return;
  }
  std::vector<ovpn_service_t> svcVec(services, services + service_count);
  std::cout << renderServiceTable(svcVec);
}

static void create_service(ovpn_mana_handle_t handle, const char *name, const char* subnet, int port)
{
  if (port <= 0 || port > 65535)
  {
    std::cerr << color::RED << "Invalid port number. Use a number between 1 and 65535" << color::RESET << std::endl;
    return;
  }
  ovpn_err_t err = ovpn_mana_create_service(handle, name, subnet, port);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to create OpenVPN service" << color::RESET << std::endl;
    return;
  }
  std::cout << color::GREEN << "OpenVPN service created successfully" << color::RESET << std::endl;
}

static void delete_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_delete_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to delete OpenVPN service" << color::RESET << std::endl;
    return;
  }
  std::cout << color::GREEN << "OpenVPN service deleted successfully" << color::RESET << std::endl;
}

static void start_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_start_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to start OpenVPN service" << color::RESET << std::endl;
    return;
  }
  std::cout << color::GREEN << "OpenVPN service started successfully" << color::RESET << std::endl;
}

static void stop_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_stop_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to stop OpenVPN service" << color::RESET << std::endl;
    return;
  }
  std::cout << color::GREEN << "OpenVPN service stopped successfully" << color::RESET << std::endl;
}

static void restart_service(ovpn_mana_handle_t handle, const char *name)
{
  ovpn_err_t err = ovpn_mana_restart_service(handle, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to restart OpenVPN service" << color::RESET << std::endl;
    return;
  }
  std::cout << color::GREEN << "OpenVPN service restarted successfully" << color::RESET << std::endl;
}

static void create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip, const char* client_ip)
{
  ovpn_err_t err;
  if (client_ip[0] != '\0')
  {
    err = ovpn_mana_create_client_with_ip(handle, service_name, name, wanip, client_ip);
  }
  else
  {
    err = ovpn_mana_create_client(handle, service_name, name, wanip);
  }
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to create OpenVPN client" << color::RESET << std::endl;
    return;
  }
  std::cout << color::GREEN << "OpenVPN client created successfully" << color::RESET << std::endl;
}

static void revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name)
{
  ovpn_err_t err = ovpn_mana_revoke_client(handle, service_name, name);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to revoke OpenVPN client" << color::RESET << std::endl;
    return;
  }
  std::cout << color::GREEN << "OpenVPN client revoked successfully" << color::RESET << std::endl;
}

static void list_online_clients(ovpn_mana_handle_t handle, const char *service_name)
{
  ovpn_client_t *clients = nullptr;
  int client_count = 0;
  ovpn_err_t err = ovpn_mana_get_online_clients(handle, service_name, nullptr, client_count);
  if (err != OVPN_ERR_SUCCESS || client_count == 0)
  {
    std::cerr << color::RED << "Failed to get online OpenVPN clients count" << color::RESET << std::endl;
    return;
  }

  clients = new ovpn_client_t[client_count];
  err = ovpn_mana_get_online_clients(handle, service_name, clients, client_count);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to get online OpenVPN clients" << color::RESET << std::endl;
    delete[] clients;
    return;
  }

  int total_count = 0;
  err = ovpn_mana_get_total_clients_count(handle, service_name, total_count);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::YELLOW << "Failed to get total clients count" << color::RESET << std::endl;
    total_count = -1;
  }

  std::vector<ovpn_client_t> cliVec(clients, clients + client_count);
  std::sort(cliVec.begin(), cliVec.end(), [](const ovpn_client_t& a, const ovpn_client_t& b) {
    uint32_t ipA = 0, ipB = 0;
    unsigned int o0, o1, o2, o3;
    if (sscanf(a.private_ipv4, "%u.%u.%u.%u", &o0, &o1, &o2, &o3) == 4)
      ipA = (o0 << 24) | (o1 << 16) | (o2 << 8) | o3;
    if (sscanf(b.private_ipv4, "%u.%u.%u.%u", &o0, &o1, &o2, &o3) == 4)
      ipB = (o0 << 24) | (o1 << 16) | (o2 << 8) | o3;
    return ipA < ipB;
  });
  std::cout << renderClientTable(cliVec, total_count >= 0 ? total_count : 0);

  delete[] clients;
}

static void export_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name)
{
  int buffer_size = 0;
  ovpn_err_t err = ovpn_mana_export_client_config(handle, service_name, name, nullptr, buffer_size);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to get client config size: error " << err << color::RESET << std::endl;
    return;
  }

  char *buffer = new char[buffer_size + 1];
  err = ovpn_mana_export_client_config(handle, service_name, name, buffer, buffer_size);
  if (err != OVPN_ERR_SUCCESS)
  {
    std::cerr << color::RED << "Failed to export client config: error " << err << color::RESET << std::endl;
    delete[] buffer;
    return;
  }

  std::cout << buffer;
  delete[] buffer;
}

static std::string exec_and_read(const std::string &cmd)
{
    std::string result;
    std::unique_ptr<FILE, int (*)(FILE *)> pipe(OVPN_POPEN(cmd.c_str(), "r"), OVPN_PCLOSE);
    if (!pipe) return result;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe.get()) != nullptr)
        result += buf;
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result;
}

static bool binary_exists(const std::string &name)
{
#ifdef OVPN_PLATFORM_WINDOWS
    std::string cmd = "where " + name + " 2>nul";
#else
    std::string cmd = "which " + name + " 2>/dev/null";
#endif
    return !exec_and_read(cmd).empty();
}

int handle_check_command()
{
    using namespace ovpn::cli;

    std::cout << "Platform: ";
#ifdef OVPN_PLATFORM_WINDOWS
    std::cout << "Windows";
#elif defined(OVPN_PLATFORM_LINUX)
    std::cout << "Linux";
#elif defined(OVPN_PLATFORM_MACOS)
    std::cout << "macOS";
#endif
    std::cout << "\n\n";

    struct CheckItem { std::string label; bool pass; std::string detail; };
    std::vector<CheckItem> items;

    bool openvpnBin = binary_exists("openvpn");
    std::string openvpnPath = "not found";
    if (openvpnBin) {
#ifdef OVPN_PLATFORM_WINDOWS
        openvpnPath = exec_and_read("where openvpn 2>nul");
#else
        openvpnPath = exec_and_read("which openvpn 2>/dev/null");
#endif
    }
    items.push_back({"OpenVPN Binary", openvpnBin, openvpnPath});

    bool easyRsaBin = binary_exists("easyrsa");
    std::string easyRsaPath = "not found";
    if (easyRsaBin) {
#ifdef OVPN_PLATFORM_WINDOWS
        easyRsaPath = exec_and_read("where easyrsa 2>nul");
#else
        easyRsaPath = exec_and_read("which easyrsa 2>/dev/null");
#endif
    }
    items.push_back({"Easy-RSA (easyrsa in PATH)", easyRsaBin, easyRsaPath});

#ifdef OVPN_PLATFORM_LINUX
    bool systemdRunning = !exec_and_read("systemctl is-active openvpn 2>/dev/null").empty();
    bool systemdEnabled = !exec_and_read("systemctl is-enabled openvpn 2>/dev/null").empty();
    items.push_back({"OpenVPN systemd Active", systemdRunning,
                     systemdRunning ? "active" : "inactive / not installed"});

    bool tunAvailable = fs::exists("/dev/net/tun");
    items.push_back({"TUN Device (/dev/net/tun)", tunAvailable,
                     tunAvailable ? "available" : "not found"});

    bool ipCmd = binary_exists("ip");
    items.push_back({"ip route tool", ipCmd, ipCmd ? exec_and_read("which ip 2>/dev/null") : "not found"});
#elif defined(OVPN_PLATFORM_WINDOWS)
    std::string svcStatus = exec_and_read("sc query OpenVPNService 2>nul");
    bool svcExists = svcStatus.find("RUNNING") != std::string::npos ||
                     svcStatus.find("STOPPED") != std::string::npos;
    std::string svcDetail = "not installed";
    if (svcExists) {
        if (svcStatus.find("RUNNING") != std::string::npos)
            svcDetail = "running";
        else if (svcStatus.find("STOPPED") != std::string::npos)
            svcDetail = "stopped";
    }
    items.push_back({"OpenVPN Windows Service", svcExists, svcDetail});

    bool tapAvailable = exec_and_read("netsh interface show interface 2>nul").find("TAP") != std::string::npos;
    items.push_back({"TAP Adapter", tapAvailable, tapAvailable ? "found" : "not found"});
#else
    items.push_back({"TUN Device", false, "manual check required on macOS"});
#endif

    int passCount = 0;
    for (const auto &item : items) {
        std::string status = item.pass ? (std::string(color::GREEN) + "OK" + color::RESET)
                                       : (std::string(color::RED) + "FAIL" + color::RESET);
        std::cout << "  [" << status << "] " << item.label;
        if (!item.detail.empty())
            std::cout << "  (" << item.detail << ")";
        std::cout << "\n";
        if (item.pass) ++passCount;
    }

    std::cout << "\n" << passCount << "/" << items.size() << " checks passed";
    if (passCount == (int)items.size())
        std::cout << "  " << color::GREEN << "All checks OK" << color::RESET;
    else
        std::cout << "  " << color::YELLOW << "Some checks failed" << color::RESET;
    std::cout << "\n";

    return 0;
}

int handle_service_command(ovpn_mana_handle_t handle, int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: openvpnmgr service <command> [options]" << std::endl;
        return -1;
    }
    std::string sub_command = argv[2];
    if (sub_command == "-l")
    {
        print_services(handle);
        return 0;
    }
    else if (sub_command == "-c")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr service -c <name>,<port>,<subnet>" << std::endl;
            return -1;
        }
        std::string service_info = argv[3];

        size_t comma_pos1 = service_info.find(',');
        size_t comma_pos2 = service_info.find(',', comma_pos1 + 1);
        if (comma_pos1 == std::string::npos || comma_pos2 == std::string::npos)
        {
            std::cerr << "Invalid service info format. Use <name>,<port>,<subnet>" << std::endl;
            return -1;
        }
        std::string name = service_info.substr(0, comma_pos1);
        std::string port_str = service_info.substr(comma_pos1 + 1, comma_pos2 - comma_pos1 - 1);
        std::string subnet = service_info.substr(comma_pos2 + 1);
        int port = std::stoi(port_str);

        create_service(handle, name.c_str(), subnet.c_str(), port);
        return 0;
    }
    else if (sub_command == "-d")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr service -d <name>" << std::endl;
            return -1;
        }
        const char *name = argv[3];
        delete_service(handle, name);
        return 0;
    }
    else if (sub_command == "-start")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr service -start <name>" << std::endl;
            return -1;
        }
        const char *name = argv[3];
        start_service(handle, name);
        return 0;
    }
    else if (sub_command == "-stop")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr service -stop <name>" << std::endl;
            return -1;
        }
        const char *name = argv[3];
        stop_service(handle, name);
        return 0;
    }
    else if (sub_command == "-restart")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr service -restart <name>" << std::endl;
            return -1;
        }
        const char *name = argv[3];
        restart_service(handle, name);
        return 0;
    }

    std::cerr << "Unknown service command: " << sub_command << std::endl;
    return -1;
}

int handle_client_command(ovpn_mana_handle_t handle, int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: openvpnmgr client <command> [options]" << std::endl;
        return -1;
    }
    std::string sub_command = argv[2];
    if (sub_command == "-c")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr client -c <service_name>,<name>,<host>[,<client_ip>]" << std::endl;
            return -1;
        }
        std::string client_info = argv[3];

        size_t comma_pos1 = client_info.find(',');
        size_t comma_pos2 = client_info.find(',', comma_pos1 + 1);
        size_t comma_pos3 = client_info.find(',', comma_pos2 + 1);
        if (comma_pos1 == std::string::npos || comma_pos2 == std::string::npos)
        {
            std::cerr << "Invalid client info format. Use <service_name>,<name>,<host>[,<client_ip>]" << std::endl;
            return -1;
        }
        std::string service_name = client_info.substr(0, comma_pos1);
        std::string name = client_info.substr(comma_pos1 + 1, comma_pos2 - comma_pos1 - 1);
        std::string wanip;
        std::string client_ip = "";
        if (comma_pos3 == std::string::npos)
        {
            wanip = client_info.substr(comma_pos2 + 1);
        }
        else
        {
            wanip = client_info.substr(comma_pos2 + 1, comma_pos3 - comma_pos2 - 1);
            client_ip = client_info.substr(comma_pos3 + 1);
        }

        create_client(handle, service_name.c_str(), name.c_str(), wanip.c_str(), client_ip.c_str());
        return 0;
    }
    else if (sub_command == "-d")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr client -d <service_name>,<name>" << std::endl;
            return -1;
        }
        std::string client_info = argv[3];
        size_t comma_pos = client_info.find(',');
        if (comma_pos == std::string::npos)
        {
            std::cerr << "Invalid client info format. Use <service_name>,<name>" << std::endl;
            return -1;
        }
        std::string service_name = client_info.substr(0, comma_pos);
        std::string name = client_info.substr(comma_pos + 1);
        revoke_client(handle, service_name.c_str(), name.c_str());
        return 0;
    }
    else if (sub_command == "-l")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr client -l <service_name>" << std::endl;
            return -1;
        }
        const char *service_name = argv[3];
        list_online_clients(handle, service_name);
        return 0;
    }
    else if (sub_command == "-export")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: openvpnmgr client -export <service_name>,<name>" << std::endl;
            return -1;
        }
        std::string client_info = argv[3];
        size_t comma_pos = client_info.find(',');
        if (comma_pos == std::string::npos)
        {
            std::cerr << "Invalid client info format. Use <service_name>,<name>" << std::endl;
            return -1;
        }
        std::string service_name = client_info.substr(0, comma_pos);
        std::string name = client_info.substr(comma_pos + 1);
        export_client_config(handle, service_name.c_str(), name.c_str());
        return 0;
    }

    std::cerr << "Unknown client command: " << sub_command << std::endl;
    return -1;
}

} // namespace ovpn::cli
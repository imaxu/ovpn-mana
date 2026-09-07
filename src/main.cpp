#include <iostream>
#include <string>
#include <cstring>
#include "ovpn-mana/ovpn_mana_api.h"
#include "ovpn-mana/ovpn_mana_platform.h"
#include "ovpn-mana/ovpn_mana_version.h"
#include "cli/console_renderer.hpp"
#include "cli/cli_handler.hpp"

int main(int argc, char *argv[])
{
  ovpn_enable_ansi_terminal();

  setlocale(LC_ALL, "zh_CN.UTF-8");

  std::cout << ovpn::cli::renderBanner("OpenVPN Command Line Manager", OVPN_VERSION_STRING);

  std::string config_path;
  for (int i = 1; i < argc - 1; i++) {
    if (std::strcmp(argv[i], "--config") == 0 || std::strcmp(argv[i], "-c") == 0) {
      config_path = argv[i + 1];
      break;
    }
  }

  if (argc == 2 && std::strcmp(argv[1], "--check") == 0)
  {
    return ovpn::cli::handle_check_command();
  }

  bool is_root = OVPN_GETUID() == 0;

  ovpn_mana_handle_t handle = ovpn_mana_create();
  if (!handle)
  {
    std::cerr << ovpn::cli::color::RED << "Failed to create OpenVPN manager" << ovpn::cli::color::RESET << std::endl;
    return -1;
  }

  if (!config_path.empty()) {
    ovpn_err_t err = ovpn_mana_load_config(handle, config_path.c_str());
    if (err != OVPN_ERR_SUCCESS) {
      std::cerr << ovpn::cli::color::RED << "Failed to load config: " << config_path << ovpn::cli::color::RESET << std::endl;
      ovpn_mana_destroy(handle);
      return -1;
    }
    std::cout << ovpn::cli::color::GREEN << "Config loaded from: " << config_path << ovpn::cli::color::RESET << std::endl;
  }

  if (!is_root) {
    std::cerr << ovpn::cli::color::RED << "Please run this command line program with ROOT privileges" << ovpn::cli::color::RESET << std::endl;
    ovpn_mana_destroy(handle);
    return -1;
  }

  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <command> [options]" << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  --check                                      Check OpenVPN environment" << std::endl;
    std::cerr << "  service -l                                  List OpenVPN services" << std::endl;
    std::cerr << "  service -c <name>,<port>,<subnet>          Create OpenVPN service" << std::endl;
    std::cerr << "  service -d <name>                           Delete OpenVPN service" << std::endl;
    std::cerr << "  service -start <name>                       Start OpenVPN service" << std::endl;
    std::cerr << "  service -stop <name>                        Stop OpenVPN service" << std::endl;
    std::cerr << "  service -restart <name>                     Restart OpenVPN service" << std::endl;
    std::cerr << "  client  -c <service_name>,<name>,<host>[,<client_ip>]  Create OpenVPN client" << std::endl;
    std::cerr << "  client  -d <service_name>,<name>            Revoke OpenVPN client" << std::endl;
    std::cerr << "  client  -l <service_name>                   List online OpenVPN clients" << std::endl;
    std::cerr << "  client  -export <service_name>,<name>        Export client .ovpn config to stdout" << std::endl;
    ovpn_mana_destroy(handle);
    return -1;
  }

  std::string command = argv[1];
  int result = -1;

  if (command == "service")
  {
    result = ovpn::cli::handle_service_command(handle, argc, argv);
  }
  else if (command == "client")
  {
    result = ovpn::cli::handle_client_command(handle, argc, argv);
  }
  else
  {
    std::cerr << "Unknown command: " << command << std::endl;
  }

  ovpn_mana_destroy(handle);
  return result;
}
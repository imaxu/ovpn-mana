#include <iostream>
#include "core/config_loader.hpp"

int main() {
    std::cout << "=== OpenVPN Manager Config Diagnostics ===" << std::endl;
    std::cout << std::endl;

    auto config = ovpn::config::load_config();

    std::cout << "Final Configuration:" << std::endl;
    std::cout << "  easy_rsa_dir:   " << config.easy_rsa_dir << std::endl;
    std::cout << "  ovpn_dir:       " << config.ovpn_dir << std::endl;
    std::cout << "  openvpn_bin:    " << config.openvpn_bin << std::endl;
    std::cout << "  systemctl_bin:  " << config.systemctl_bin << std::endl;

    std::cout << std::endl;
    std::cout << "=== End Diagnostics ===" << std::endl;

    return 0;
}
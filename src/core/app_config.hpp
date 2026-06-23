#pragma once
#include <string>

struct AppConfig {
    std::string easy_rsa_dir;
    std::string ovpn_dir;
    std::string openvpn_bin;
    std::string systemctl_bin;

    std::string ovpn_server_conf_dir() const { return ovpn_dir + "/server"; }
    std::string client_configs_dir()  const { return ovpn_dir + "/client-configs"; }

    static AppConfig defaults() {
        return AppConfig{
            "/etc/openvpn/easy-rsa",
            "/etc/openvpn",
            "/usr/sbin/openvpn",
            "/bin/systemctl"
        };
    }
};
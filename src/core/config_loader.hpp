#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

#include "app_config.hpp"

namespace ovpn::config {

inline std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

inline AppConfig load_from_file(const std::string &config_path) {
    AppConfig config = AppConfig::defaults();

    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "[CONFIG] WARNING: Cannot open config file: " << config_path << std::endl;
        std::cerr << "[CONFIG] Using default configuration" << std::endl;
        return config;
    }

    std::cerr << "[CONFIG] Successfully opened: " << config_path << std::endl;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    if (content.empty()) {
        std::cerr << "[CONFIG] WARNING: Config file is empty: " << config_path << std::endl;
        return config;
    }

    std::cerr << "[CONFIG] File size: " << content.length() << " bytes" << std::endl;

    auto get_value = [&content](const std::string &key) -> std::string {
        std::string search_key = "\"" + key + "\"";
        size_t pos = content.find(search_key);
        if (pos == std::string::npos) return "";

        pos = content.find(':', pos);
        if (pos == std::string::npos) return "";

        pos++;
        size_t start = content.find('"', pos);
        if (start == std::string::npos) return "";

        start++;
        size_t end = content.find('"', start);
        if (end == std::string::npos) return "";

        return content.substr(start, end - start);
    };

    std::string value;

    value = get_value("easy_rsa_dir");
    if (!value.empty()) {
        config.easy_rsa_dir = value;
        std::cerr << "[CONFIG] Parsed easy_rsa_dir: " << value << std::endl;
    } else {
        std::cerr << "[CONFIG] easy_rsa_dir not found in config, using default" << std::endl;
    }

    value = get_value("ovpn_dir");
    if (!value.empty()) {
        config.ovpn_dir = value;
        std::cerr << "[CONFIG] Parsed ovpn_dir: " << value << std::endl;
    }

    value = get_value("openvpn_bin");
    if (!value.empty()) {
        config.openvpn_bin = value;
        std::cerr << "[CONFIG] Parsed openvpn_bin: " << value << std::endl;
    }

    value = get_value("systemctl_bin");
    if (!value.empty()) {
        config.systemctl_bin = value;
        std::cerr << "[CONFIG] Parsed systemctl_bin: " << value << std::endl;
    }

    std::cerr << "[CONFIG] Configuration loaded successfully from file" << std::endl;
    return config;
}

inline AppConfig load_with_env_override(AppConfig base_config) {
    const char* env_easy_rsa = std::getenv("OVPN_EASY_RSA_DIR");
    if (env_easy_rsa && env_easy_rsa[0] != '\0') {
        base_config.easy_rsa_dir = env_easy_rsa;
    }

    const char* env_ovpn_dir = std::getenv("OVPN_DIR");
    if (env_ovpn_dir && env_ovpn_dir[0] != '\0') {
        base_config.ovpn_dir = env_ovpn_dir;
    }

    const char* env_openvpn = std::getenv("OVPN_OPENVPN_BIN");
    if (env_openvpn && env_openvpn[0] != '\0') {
        base_config.openvpn_bin = env_openvpn;
    }

    const char* env_systemctl = std::getenv("OVPN_SYSTEMCTL_BIN");
    if (env_systemctl && env_systemctl[0] != '\0') {
        base_config.systemctl_bin = env_systemctl;
    }

    return base_config;
}

inline AppConfig load_config(const std::string& config_path = "") {
    std::string actual_path = config_path;

    if (actual_path.empty()) {
#ifdef WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string exe_dir(buffer);
        size_t last_sep = exe_dir.find_last_of("\\/");
        if (last_sep != std::string::npos) {
            exe_dir = exe_dir.substr(0, last_sep);
        }
        actual_path = exe_dir + "\\ovpn-mana.json";
#else
        actual_path = "/etc/openvpn/ovpn-mana.json";
#endif
    }

    AppConfig config = load_from_file(actual_path);
    config = load_with_env_override(config);

    return config;
}

}
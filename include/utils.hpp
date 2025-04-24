#ifndef _UTILS_HPP
#define _UTILS_HPP

#include "sdk.types.hpp"
#include <memory>
#include <array>

// 从ini文件中读取配置，并返回 ovpn_config_t
std::unique_ptr<ovpn_config_t> read_ini(const std::string& filename) {

    // 读取配置文件
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开配置文件: " << filename << std::endl;
        return nullptr;
    }

    bool is_server_section = false;
    bool is_cli_section = false;
    auto config = std::make_unique<ovpn_config_t>();
    std::string line;
    while (std::getline(file, line)) {
      // 去掉行首尾的空格
      line.erase(0, line.find_first_not_of(" \t"));
      line.erase(line.find_last_not_of(" \t") + 1);

      // 跳过空行和注释
      if (line.empty() || line[0] == '#') {
        continue;
      }

      // 解析节名
      if (line[0] == '[' && line.back() == ']') {
        std::string section = line.substr(1, line.size() - 2);
        if (section == "server") {
          is_server_section = true;
        } else if (section == "cli") {
          is_cli_section = true;
        }
        continue;
      }

      // 解析键值对
      auto delimiter_pos = line.find('=');
      if (delimiter_pos != std::string::npos) {
        std::string key = line.substr(0, delimiter_pos);
        std::string value = line.substr(delimiter_pos + 1);

        // 去掉键和值的空格
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 根据节名填充配置
        if (is_server_section) {
          if (key == "server_host") {
            config->server_host = value;
          } else if (key == "server_port") {
            config->server_port = value;
          }
        } else if (is_cli_section) {
          if (key == "status_file") {
            config->status_file = value;
          } else if (key == "ca_file") {
            config->ca_file = value;
          } else if (key == "issue_cert_file") {
            config->issue_cert_file = value;
          } else if (key == "private_key_file") {
            config->private_key_file = value;
          } else if (key == "ta_key_file") {
            config->ta_key_file = value;
          } else if (key == "server_config_file") {
            config->server_config_file = value;
          }
        }
      }
    }

    return config;
}

#endif // !_UTILS_HPP
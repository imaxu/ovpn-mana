#include "core/command_templates.hpp"
#include <map>

namespace ovpn::commands {

std::string replace(
    std::string_view tmpl,
    const AppConfig& cfg,
    std::initializer_list<std::pair<std::string_view, std::string>> params)
{
    std::map<std::string, std::string> replacements;
    replacements["EASY_RSA_DIR"]         = cfg.easy_rsa_dir;
    replacements["OVPN_DIR"]             = cfg.ovpn_dir;
    replacements["OPENVPN_BIN"]          = cfg.openvpn_bin;
    replacements["SYSTEMCTL_BIN"]        = cfg.systemctl_bin;
    replacements["OVPN_SERVER_CONF_DIR"] = cfg.ovpn_server_conf_dir();
    replacements["CLIENT_CONFIGS_DIR"]   = cfg.client_configs_dir();

    for (const auto& [key, value] : params) {
        replacements[std::string(key)] = value;
    }

    std::string result(tmpl);
    for (const auto& [key, value] : replacements) {
        std::string placeholder = "{" + key + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    return result;
}

} // namespace ovpn::commands
#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <initializer_list>
#include "app_config.hpp"

namespace ovpn::commands {

constexpr std::string_view EASYRSA_GEN_REQ           = "cd {EASY_RSA_DIR} && easyrsa --batch gen-req {NAME}";
constexpr std::string_view EASYRSA_SIGN_REQ_SERVER   = "cd {EASY_RSA_DIR} && easyrsa --batch sign-req server {NAME}";
constexpr std::string_view EASYRSA_BUILD_CLIENT_FULL = "cd {EASY_RSA_DIR} && easyrsa --batch build-client-full {NAME} nopass";
constexpr std::string_view EASYRSA_REVOKE            = "cd {EASY_RSA_DIR} && easyrsa --batch revoke {NAME}";
constexpr std::string_view EASYRSA_GEN_CRL           = "cd {EASY_RSA_DIR} && easyrsa gen-crl";
constexpr std::string_view EASYRSA_GEN_DH            = "cd {EASY_RSA_DIR} && easyrsa gen-dh";

constexpr std::string_view SYSTEMCTL_START      = "{SYSTEMCTL_BIN} start openvpn@{NAME}";
constexpr std::string_view SYSTEMCTL_STOP       = "sudo {SYSTEMCTL_BIN} stop openvpn@{NAME}";
constexpr std::string_view SYSTEMCTL_RESTART    = "{SYSTEMCTL_BIN} restart openvpn@{NAME}";
constexpr std::string_view SYSTEMCTL_ENABLE     = "{SYSTEMCTL_BIN} enable openvpn@{NAME}";
constexpr std::string_view SYSTEMCTL_DISABLE    = "{SYSTEMCTL_BIN} disable openvpn@{NAME}";
constexpr std::string_view SYSTEMCTL_IS_ACTIVE  = "{SYSTEMCTL_BIN} is-active openvpn@{NAME}";
constexpr std::string_view SYSTEMCTL_IS_ENABLED = "{SYSTEMCTL_BIN} is-enabled openvpn@{NAME}";

constexpr std::string_view OPENVPN_GEN_TA_KEY = "sudo {OPENVPN_BIN} --genkey secret {OUTPUT_PATH}";

constexpr std::string_view CP_WITH_SUDO = "sudo cp {SRC} {DEST}";
constexpr std::string_view CHMOD        = "sudo chmod {MODE} {PATH}";

constexpr std::string_view SERVER_CONFIG = R"(topology subnet
port {PORT}
proto udp
dev tun
ca {SERVER_DIR}/ca.crt
cert {SERVER_DIR}/server.crt
key {SERVER_DIR}/server.key
dh {SERVER_DIR}/dh.pem
tls-auth {SERVER_DIR}/ta.key 0
server {SUBNET} 255.255.255.0
keepalive 10 120
persist-key
persist-tun
ifconfig-pool-persist {SERVER_DIR}/ipp.txt
client-config-dir {SERVER_DIR}/ccd
status {SERVER_DIR}/status.log
verb 3
)";

constexpr std::string_view CLIENT_CONFIG = R"(client
dev tun
proto udp
remote {WAN_IP} {PORT}
resolv-retry infinite
nobind
persist-key
persist-tun
remote-cert-tls server
cipher AES-256-CBC
verb 3
)";

std::string replace(
    std::string_view tmpl,
    const AppConfig& cfg,
    std::initializer_list<std::pair<std::string_view, std::string>> params = {});

} // namespace ovpn::commands
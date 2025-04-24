#ifndef _SDK_TYPES_HPP
#define _SDK_TYPES_HPP


typedef int ovpn_err_t;

typedef void* ovpn_mana_handle_t;

typedef struct ovpn_server_config_t {

  const char* server_host; // 服务器地址
  uint32_t server_port; // 服务器端口

} ovpn_server_config_t;

typedef struct ovpn_cli_config_t {

  const char* status_file; // 状态文件路径
  const char* ca_file;  // CA证书路径
  const char* issue_cert_file;  // 发行证书路径
  const char* private_key_file; // 私钥路径
  const char* ta_key_file;  // TLS认证密钥路径
  const char* server_config_file; // 服务配置文件路径
}

typedef struct ovpn_config_t {

  ovpn_server_config_t server; // 服务器配置
  ovpn_cli_config_t cli; // OPENVPN服务运行时配置


} ovpn_config_t;


typedef struct ovpn_client_t{

  const char* name; // 客户端名称
  const char* vpn_ip; // VPN IP地址
  const char* real_ip; // 实际IP地址
  const char* since; // 连接时间
  uint64_t bytes_received; // 接收字节数
  uint64_t bytes_sent; // 发送字节数

} ovpn_client_t;


typedef struct ovpn_service_conf_t {

  uint32_t port;
  const char* proto;
  const char* status_file; // 状态文件路径
  const char* ca_file;  // CA证书路径
  const char* service_cert_file;  // 发行证书路径
  const char* service_key_file; // 私钥路径
  const char* dh_file; // DH PEM
  const char* ta_key_file;  // TLS认证密钥路径


} ovpn_service_conf_t;

#endif // !1
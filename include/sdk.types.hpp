#ifndef _SDK_TYPES_HPP
#define _SDK_TYPES_HPP


typedef int ovpn_err_t;

typedef void* ovpn_mana_handle_t;


typedef struct {

    char name[64];         // 服务名称
    char configPath[256];  // 配置路径
    int is_activated;          // 是否激活
    int is_enabled;         // 是否自启

} ovpn_service_t;


typedef struct {

    char name[128];         // 客户端名称
    char private_ipv4[32];       // VPN IP地址
    char public_ipv4[64];      // 实际IP地址
    char since[64];       // 连接时间
    unsigned long long bytes_received; // 接收字节数
    unsigned long long bytes_sent;     // 发送字节数

} ovpn_client_t;

/* 错误码 */
#define OVPN_ERR_SUCCESS 0
#define OVPN_ERR_FAILURE -1
#define OVPN_ERR_INVALID_PARAM -2
#define OVPN_ERR_NOT_FOUND -3
#define OVPN_ERR_PERMISSION_DENIED -4
#define OVPN_ERR_TIMEOUT -5
#define OVPN_ERR_IO_FAILURE -6

#endif // !1
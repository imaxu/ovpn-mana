#ifndef OVPN_MANA_TYPES_H
#define OVPN_MANA_TYPES_H

typedef int ovpn_err_t;

typedef void* ovpn_mana_handle_t;

typedef struct {
    char name[64];
    char configPath[256];
    int port;
    char subnet[32];
    int is_activated;
    int is_enabled;
} ovpn_service_t;

typedef struct {
    char name[128];
    char private_ipv4[32];
    char public_ipv4[64];
    char since[64];
    unsigned long long bytes_received;
    unsigned long long bytes_sent;
} ovpn_client_t;

typedef struct {
    char easy_rsa_dir[256];
    char ovpn_dir[256];
    char openvpn_bin[256];
    char systemctl_bin[256];
} ovpn_config_t;

#define OVPN_ERR_SUCCESS              0
#define OVPN_ERR_FAILURE             -1
#define OVPN_ERR_INVALID_PARAM       -2
#define OVPN_ERR_NOT_FOUND           -3
#define OVPN_ERR_PERMISSION_DENIED   -4
#define OVPN_ERR_TIMEOUT             -5
#define OVPN_ERR_IO_FAILURE          -6
#define OVPN_ERR_NAME_TOO_LONG     -1002
#define OVPN_ERR_IP_FORMAT         -1003
#define OVPN_ERR_PORT_RANGE        -1004
#define OVPN_ERR_FORBIDDEN_CHAR    -1005
#define OVPN_ERR_BUFFER_TOO_SMALL  -1006
#define OVPN_ERR_SERVICE_NOT_FOUND -1007
#define OVPN_ERR_CLIENT_NOT_FOUND  -1008
#define OVPN_ERR_IP_CONFLICT       -1009

#endif
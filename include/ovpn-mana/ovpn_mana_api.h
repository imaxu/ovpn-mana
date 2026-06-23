#ifndef OVPN_MANA_API_H
#define OVPN_MANA_API_H

#include "ovpn_mana_types.h"
#include "ovpn_mana_platform.h"

#if defined(OVPN_PLATFORM_WINDOWS)
#define LIB_API _declspec(dllexport)
#define LIB_API_CALL __stdcall
#else
#define LIB_API
#define LIB_API_CALL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  LIB_API ovpn_mana_handle_t LIB_API_CALL ovpn_mana_create();

  LIB_API void LIB_API_CALL ovpn_mana_destroy(ovpn_mana_handle_t handle);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_list_services(ovpn_mana_handle_t handle, ovpn_service_t *services, int &service_count);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_service(ovpn_mana_handle_t handle, const char *name, const char* subnet, int port);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_start_service(ovpn_mana_handle_t handle, const char *name);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_stop_service(ovpn_mana_handle_t handle, const char *name);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_restart_service(ovpn_mana_handle_t handle, const char *name);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_delete_service(ovpn_mana_handle_t handle, const char *name);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client_with_ip(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip, const char* client_ip);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_export_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name, char *buffer, int &buffer_size);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_online_clients(ovpn_mana_handle_t handle, const char *service_name, ovpn_client_t *clients, int &client_count);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_total_clients_count(ovpn_mana_handle_t handle, const char *service_name, int &total_count);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name, char *ovpn_file, int &ovpn_file_size);

  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_configure(ovpn_mana_handle_t handle, const ovpn_config_t *config);

  LIB_API const char* LIB_API_CALL ovpn_mana_get_version();

#ifdef __cplusplus
}
#endif

#endif
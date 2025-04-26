/**
 * @file ovpn-mana.hpp
 * @brief OpenVPN管理器动态库头文件
 * @author xuwh (imaxu@sina.com)
 * @date 2025-04-26
 * @version 1.0
 * @note 一系列支持对OpenVPN服务进行操作的API
 */

#ifndef OVPN_MANA_HPP
#define OVPN_MANA_HPP

#include "sdk.types.hpp"

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
// WINDOWS
#define LIB_API _declspec(dllexport)
#define LIB_API_CALL __stdcall
#define WIN

#else
// LINUX
#define LIB_API
#define LIB_API_CALL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  /// @brief  创建OpenVPN管理器实例并返回句柄
  /// @return  句柄
  /// @note    该函数会创建一个OpenVPN管理器实例，并返回一个句柄。该句柄可以用于后续的操作。
  LIB_API ovpn_mana_handle_t LIB_API_CALL ovpn_mana_create();

  /// @brief  销毁OpenVPN管理器实例
  /// @param  handle  句柄
  /// @note    该函数会销毁OpenVPN管理器实例，并释放相关资源。
  LIB_API void LIB_API_CALL ovpn_mana_destroy(ovpn_mana_handle_t handle);

  /// @brief  获取OpenVPN服务列表
  /// @param  handle  句柄
  /// @param  services  服务列表
  /// @param  count  服务数量
  /// @return  错误码
  /// @note    该函数会获取OpenVPN服务列表，并返回服务数量。服务列表中的每个服务包含名称、配置路径、是否激活和是否自启等信息。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_list_services(ovpn_mana_handle_t handle, ovpn_service_t *services, int &service_count);

  /// @brief  创建OpenVPN服务
  /// @param  handle  句柄
  /// @param  name  服务名称
  /// @param  port  服务端口
  /// @return  错误码
  /// @note    该函数会创建一个OpenVPN服务，并返回错误码。服务名称和端口号必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_service(ovpn_mana_handle_t handle, const char *name, const char* subnet, int port);

  /// @brief  启动OpenVPN服务
  /// @param  handle  句柄
  /// @param  name  服务名称
  /// @return  错误码
  /// @note    该函数会启动一个OpenVPN服务，并返回错误码。服务名称必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_start_service(ovpn_mana_handle_t handle, const char *name);

  /// @brief  停止OpenVPN服务
  /// @param  handle  句柄
  /// @param  name  服务名称
  /// @return  错误码
  /// @note    该函数会停止一个OpenVPN服务，并返回错误码。服务名称必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_stop_service(ovpn_mana_handle_t handle, const char *name);

  /// @brief  重启OpenVPN服务
  /// @param  handle  句柄
  /// @param  name  服务名称
  /// @return  错误码
  /// @note    该函数会重启一个OpenVPN服务，并返回错误码。服务名称必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_restart_service(ovpn_mana_handle_t handle, const char *name);

  /// @brief  删除OpenVPN服务
  /// @param  handle  句柄
  /// @param  name  服务名称
  /// @return  错误码
  /// @note    该函数会删除一个OpenVPN服务，并返回错误码。服务名称必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_delete_service(ovpn_mana_handle_t handle, const char *name);

  /// @brief  创建OpenVPN客户端
  /// @param  handle  句柄
  /// @param  service_name  服务名称
  /// @param  name  客户端名称
  /// @return  错误码
  /// @note    该函数会创建一个OpenVPN客户端，并返回错误码。客户端名称和服务名称必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip);

  /// @brief  吊销OpenVPN客户端
  /// @param  handle  句柄
  /// @param  service_name  服务名称
  /// @param  name  客户端名称
  /// @return  错误码
  /// @note    该函数会吊销一个OpenVPN客户端，并返回错误码。客户端名称和服务名称必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name);

  /// @brief  获取在线OpenVPN客户端列表
  /// @param  handle  句柄
  /// @param  service_name  服务名称
  /// @param  clients  客户端列表
  /// @param  client_count  客户端数量
  /// @return  错误码
  /// @note    该函数会获取在线OpenVPN客户端列表，并返回客户端数量。客户端列表中的每个客户端包含名称、VPN IP、真实 IP、上线时间、接收字节数和发送字节数等信息。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_online_clients(ovpn_mana_handle_t handle, const char *service_name, ovpn_client_t *clients, int &client_count);

  /// @brief  返回指定OpenVPN客户端配置文件内容
  /// @param  handle  句柄
  /// @param  service_name  服务名称
  /// @param  name  客户端名称
  /// @param  ovpn_file  客户端配置文件内容
  /// @param  ovpn_file_size  客户端配置文件大小
  /// @return  错误码
  /// @note    该函数会返回指定OpenVPN客户端配置文件内容，并返回配置文件大小。客户端名称和服务名称必须合法。
  LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_get_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name, char *ovpn_file, int &ovpn_file_size);


#ifdef __cplusplus
}
#endif

#endif // OVPN_MANA_HPP
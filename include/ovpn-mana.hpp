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


/// @brief 创建OpenVPN管理器实例
/// @param config OpenVPN配置
/// @return 返回OpenVPN管理器实例句柄
/// @note 该函数会读取配置文件，初始化OpenVPN管理器实例
LIB_API ovpn_mana_handle_t LIB_API_CALL ovpn_mana_create(ovpn_config_t& config);

/// @brief 销毁OpenVPN管理器实例
/// @param handle OpenVPN管理器实例句柄
/// @return 返回错误码
/// @note 该函数会释放OpenVPN管理器实例的资源
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_destroy(ovpn_mana_handle_t handle);

/// @brief  获取OpenVPN客户端列表
/// @param handle  OpenVPN管理器实例句柄
/// @param clients  客户端列表
/// @param client_count  客户端数量
/// @return  返回错误码
/// @note 该函数会获取OpenVPN客户端列表
LIB_API ovpn_err_t LIB_API_CALL ovpn_list_clients(ovpn_mana_handle_t handle, ovpn_client_t& clients, int& client_count);


/// @brief  创建OpenVPN客户端
/// @param handle  OpenVPN管理器实例句柄
/// @param name  客户端名称
/// @return  返回错误码
/// @note 该函数会创建OpenVPN客户端
LIB_API ovpn_err_t LIB_API_CALL ovpn_create_client(ovpn_mana_handle_t handle, const char* name);

/// @brief  吊销OpenVPN客户端 (**重启服务生效**)
/// @param handle  OpenVPN管理器实例句柄
/// @param name  客户端名称
/// @return  返回错误码
/// @note 该函数会吊销OpenVPN客户端
LIB_API ovpn_err_t LIB_API_CALL ovpn_revoke_client(ovpn_mana_handle_t handle, const char* name);

/// @brief  启动OpenVPN服务
/// @param handle  OpenVPN管理器实例句柄
/// @param svc_name  服务名称（**确保 服务名称.conf 存在**)
/// @return  返回错误码
/// @note 该函数会启动OpenVPN服务
LIB_API ovpn_err_t LIB_API_CALL ovpn_start_service(ovpn_mana_handle_t handle, const char* svc_name);

/// @brief  停止OpenVPN服务
/// @param handle  OpenVPN管理器实例句柄
/// @param svc_name  服务名称（**确保 服务名称.conf 存在**)
/// @return  返回错误码
/// @note 该函数会停止OpenVPN服务
LIB_API ovpn_err_t LIB_API_CALL ovpn_stop_service(ovpn_mana_handle_t handle, const char* svc_name);

/// @brief  重启OpenVPN服务
/// @param handle  OpenVPN管理器实例句柄
/// @param svc_name  服务名称（**确保 服务名称.conf 存在**)
/// @return  返回错误码
/// @note 该函数会重启OpenVPN服务
LIB_API ovpn_err_t LIB_API_CALL ovpn_restart_service(ovpn_mana_handle_t handle, const char* svc_name);


#ifdef __cplusplus
}
#endif

#endif // OVPN_MANA_HPP
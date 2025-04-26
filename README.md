## OVPN-MANA

### APIs

##### 基础接口

##### 创建管理器实例

> 该接口创建一个管理器实例，并返回其内存指针供后续使用。

```cpp
  ovpn_mana_handle_t ovpn_mana_create();
```

##### 销毁管理器实例

> 该接口将销毁管理器实例并释放内存资源。

```cpp
  void ovpn_mana_destroy(ovpn_mana_handle_t handle);
```

#### 服务管理接口

##### 获取OpenVPN服务列表

> 通过该接口获取当前服务列表。（ 注意：仅返回由OVPN-MANA创建的服务 ）

```cpp
  ovpn_err_t ovpn_mana_list_services(ovpn_mana_handle_t handle, ovpn_service_t *services, int &service_count);
```

###### 参数

| 字段名        | 类型                 | 说明                     |
| ------------- | -------------------- | ------------------------ |
| handle        | `ovpn_mana_handle_t` | 管理器实例指针           |
| services      | `ovpn_service_t *`   | 返回的服务列表首地址指针 |
| service_count | `int&`               | 返回的服务列表长度       |

`ovpn_service_t`

| 字段名       | 类型        | 说明     |
| ------------ | ----------- | -------- |
| name         | `char[64]`  | 服务名称 |
| configPath   | `char[256]` | 配置路径 |
| is_activated | `int`       | 是否激活 |
| is_enabled   | `int`       | 是否自启 |

##### 创建OpenVPN服务

> 创建OpenVPN服务实例，可指定名称、子网范围和端口。 （注意： 确保端口合法且不未被占用）

```cpp
  ovpn_err_t ovpn_mana_create_service(ovpn_mana_handle_t handle, const char *name, const char* subnet, int port);
```

###### 参数
| 字段名 | 类型                 | 说明                   |
| ------ | -------------------- | ---------------------- |
| handle | `ovpn_mana_handle_t` | 管理器实例指针         |
| name   | `const char *`       | 服务名称               |
| subnet | `const char*`        | 子网IPv4，如 172.1.0.0 |
| port   | `int`                | 服务端口               |

##### 启动OpenVPN服务

```cpp
  ovpn_err_t ovpn_mana_start_service(ovpn_mana_handle_t handle, const char *name)
```
###### 参数
| 字段名 | 类型                 | 说明           |
| ------ | -------------------- | -------------- |
| handle | `ovpn_mana_handle_t` | 管理器实例指针 |
| name   | `const char *`       | 服务名称       |


##### 停止OpenVPN服务

```cpp
  ovpn_err_t ovpn_mana_stop_service(ovpn_mana_handle_t handle, const char *name)
```
###### 参数
| 字段名 | 类型                 | 说明           |
| ------ | -------------------- | -------------- |
| handle | `ovpn_mana_handle_t` | 管理器实例指针 |
| name   | `const char *`       | 服务名称       |
##### 重启OpenVPN服务

```cpp
  ovpn_err_t ovpn_mana_restart_service(ovpn_mana_handle_t handle, const char *name)
```
###### 参数
| 字段名 | 类型                 | 说明           |
| ------ | -------------------- | -------------- |
| handle | `ovpn_mana_handle_t` | 管理器实例指针 |
| name   | `const char *`       | 服务名称       |

##### 删除OpenVPN服务

```cpp
  ovpn_err_t ovpn_mana_delete_service(ovpn_mana_handle_t handle, const char *name)
```
###### 参数
| 字段名 | 类型                 | 说明           |
| ------ | -------------------- | -------------- |
| handle | `ovpn_mana_handle_t` | 管理器实例指针 |
| name   | `const char *`       | 服务名称       |

#### 客户端管理接口

##### 获取在线客户端列表

```cpp
  ovpn_err_t ovpn_mana_get_online_clients(ovpn_mana_handle_t handle, const char *service_name, ovpn_client_t *clients, int &client_count)
```
###### 参数
| 字段名       | 类型                 | 说明                       |
| ------------ | -------------------- | -------------------------- |
| handle       | `ovpn_mana_handle_t` | 管理器实例指针             |
| service_name | `const char *`       | 服务名称                   |
| clients      | `ovpn_client_t *`    | 返回的客户端列表首地址指针 |
| client_count | `int&`               | 返回的客户端列表长度       |

##### `ovpn_client_t`

| 字段名         | 类型                 | 说明       |
| -------------- | -------------------- | ---------- |
| name           | `char[64]`           | 客户端名称 |
| private_ipv4   | `char[16]`           | VPN IP地址 |
| public_ipv4    | `char[16]`           | 实际IP地址 |
| since          | `char[32]`           | 连接时间   |
| bytes_received | `unsigned long long` | 接收字节数 |
| bytes_sent     | `unsigned long long` | 发送字节数 |

##### 创建客户端

```cpp
  ovpn_err_t ovpn_mana_create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip);
```
###### 参数
| 字段名       | 类型                 | 说明                 |
| ------------ | -------------------- | -------------------- |
| handle       | `ovpn_mana_handle_t` | 管理器实例指针       |
| service_name | `const char *`       | 服务名称             |
| name         | `const char*`        | 客户端名称           |
| wanip        | `const char*`        | 客户端访问的公网地址 |

>##### 吊销客户端

```cpp
  ovpn_err_t ovpn_mana_revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name)
```
###### 参数
| 字段名       | 类型                 | 说明           |
| ------------ | -------------------- | -------------- |
| handle       | `ovpn_mana_handle_t` | 管理器实例指针 |
| service_name | `const char *`       | 服务名称       |
| name         | `const char *`       | 客户端名称     |

##### 获取客户端OVPN文件内容

```cpp
  ovpn_err_t ovpn_mana_get_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name, char *ovpn_file, int &ovpn_file_size);
```

###### 参数
| 字段名         | 类型                 | 说明                                                    |
| -------------- | -------------------- | ------------------------------------------------------- |
| handle         | `ovpn_mana_handle_t` | 管理器实例指针                                          |
| service_name   | `const char *`       | 服务名称                                                |
| name           | `const char*`        | 客户端名称                                              |
| ovpn_file      | `char *`             | 客户端OVPN配置文件内容，调用方分配指针，建议大小10K以上 |
| ovpn_file_size | `int&`               | 客户端OVPN配置文件实际大小                              |


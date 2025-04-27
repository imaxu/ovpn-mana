## OVPN-MANA

An OpenVPN command-line C++ library and a simple management program based on the library.

[![Supported Versions](https://img.shields.io/badge/cxx-17-blue)](https://github.com/imaxu/ovpn-mana.git)
[![Supported Versions](https://img.shields.io/badge/easy__rsa-3.2.2+-blue)](https://github.com/OpenVPN/easy-rsa.git)
[![Supported Versions](https://img.shields.io/badge/openvpn-2.6+-blue)](https://github.com/OpenVPN/easy-rsa.git)
[![Supported Versions](https://img.shields.io/badge/version-1.0.0.1-blue)](https://github.com/imaxu/ovpn-mana.git)
[![Supported Versions](https://img.shields.io/badge/CI-pass-green)](https://github.com/imaxu/ovpn-mana.git)

### Environment

<font color="orange">Note: The current version requires ```ROOT``` privileges to run correctly. If you need to call it from a web application, you will need to resolve the privilege escalation issue yourself.</font>

#### Linux/Ubuntu/Debian

```bash
sudo apt update
sudo apt install openvpn easy-rsa
```

#### Windows

openvpn easy-rsa

#### Clone Repository

```shell
git clone https://github.com/imaxu/ovpn-mana.git # Clone the repository locally
cd ovpn-mana
```

#### Set Environment Variables

Modify the following paths in `CMakeLists.txt` to your actual paths:

```cmake
# Configurable compilation parameters
set(EASY_RSA_DIR "<YOUR easy-rsa PATH>" CACHE PATH "Easy-RSA")
set(OVPN_DIR "<YOUR openvpn PATH>" CACHE PATH "OpenVPNPath")
```

Or during compilation:

```shell
cd ${PROJECT_SRC}
rm ./include/config.hpp
cmake -D EASY_RSA_DIR="<YOUR easy-rsa PATH>" -D OVPN_DIR="<YOUR openvpn PATH>"
```

#### Compile

```bash
mkdir build && cd build
cmake .. && make
```

### Project Structure

```
source
├── include                     # Header file directory
│   ├── config.hpp
│   ├── OpenVPNManager.hpp      # Header file for the manager
│   ├── ovpn-mana.hpp           # Header file for the exported library
│   └── sdk.types.hpp           # Header file for library type definitions
├── src                         # Source code directory
│   ├── main.cpp                # Implementation program for openvpnmgr
│   ├── OpenVPNManager.cpp      # Implementation code for the manager
│   └── ovpn-mana.cpp           # Source code for the exported library
├── test                        # Test program directory
└── CMakeLists.txt              # CMake build script
```

### Library APIs

##### Basic Interfaces

##### Create Manager Instance

> This interface creates a manager instance and returns its memory pointer for subsequent use.

```cpp
ovpn_mana_handle_t ovpn_mana_create();
```

##### Destroy Manager Instance

> This interface destroys the manager instance and releases memory resources.

```cpp
void ovpn_mana_destroy(ovpn_mana_handle_t handle);
```

#### Service Management Interfaces

##### Get OpenVPN Service List

> This interface retrieves the current service list. (Note: Only returns services created by OVPN-MANA)

```cpp
ovpn_err_t ovpn_mana_list_services(ovpn_mana_handle_t handle, ovpn_service_t *services, int &service_count);
```

###### Parameters

| Field Name      | Type                 | Description                          |
| --------------- | -------------------- | ------------------------------------ |
| handle          | `ovpn_mana_handle_t` | Manager instance pointer             |
| services        | `ovpn_service_t *`   | Pointer to the start address of the returned service list |
| service_count | `int&`               | Length of the returned service list  |

`ovpn_service_t`

| Field Name   | Type        | Description   |
| ---------- | ----------- | ------------- |
| name       | `char[64]`  | Service name  |
| configPath | `char[256]` | Configuration path |
| is_activated | `int`       | Is activated  |
| is_enabled | `int`       | Is auto-start |

##### Create OpenVPN Service

> Creates an OpenVPN service instance, allowing you to specify the name, subnet range, and port. (Note: Ensure the port is valid and not in use)

```cpp
ovpn_err_t ovpn_mana_create_service(ovpn_mana_handle_t handle, const char *name, const char* subnet, int port);
```

###### Parameters

| Field Name | Type                 | Description                      |
| -------- | -------------------- | -------------------------------- |
| handle   | `ovpn_mana_handle_t` | Manager instance pointer         |
| name     | `const char *`       | Service name                     |
| subnet   | `const char*`        | Subnet IPv4, e.g., 172.1.0.0     |
| port     | `int`                | Service port                     |

##### Start OpenVPN Service

```cpp
ovpn_err_t ovpn_mana_start_service(ovpn_mana_handle_t handle, const char *name);
```

###### Parameters

| Field Name | Type                 | Description          |
| -------- | -------------------- | -------------------- |
| handle   | `ovpn_mana_handle_t` | Manager instance pointer |
| name     | `const char *`       | Service name         |

##### Stop OpenVPN Service

```cpp
ovpn_err_t ovpn_mana_stop_service(ovpn_mana_handle_t handle, const char *name);
```

###### Parameters

| Field Name | Type                 | Description          |
| -------- | -------------------- | -------------------- |
| handle   | `ovpn_mana_handle_t` | Manager instance pointer |
| name     | `const char *`       | Service name         |

##### Restart OpenVPN Service

```cpp
ovpn_err_t ovpn_mana_restart_service(ovpn_mana_handle_t handle, const char *name);
```

###### Parameters

| Field Name | Type                 | Description          |
| -------- | -------------------- | -------------------- |
| handle   | `ovpn_mana_handle_t` | Manager instance pointer |
| name     | `const char *`       | Service name         |

##### Delete OpenVPN Service

```cpp
ovpn_err_t ovpn_mana_delete_service(ovpn_mana_handle_t handle, const char *name);
```

###### Parameters

| Field Name | Type                 | Description          |
| -------- | -------------------- | -------------------- |
| handle   | `ovpn_mana_handle_t` | Manager instance pointer |
| name     | `const char *`       | Service name         |

#### Client Management Interfaces

##### Get Online Client List

```cpp
ovpn_err_t ovpn_mana_get_online_clients(ovpn_mana_handle_t handle, const char *service_name, ovpn_client_t *clients, int &client_count);
```

###### Parameters

| Field Name     | Type                 | Description                            |
| -------------- | -------------------- | -------------------------------------- |
| handle         | `ovpn_mana_handle_t` | Manager instance pointer               |
| service_name   | `const char *`       | Service name                           |
| clients        | `ovpn_client_t *`    | Pointer to the start address of the returned client list |
| client_count | `int&`               | Length of the returned client list     |

##### `ovpn_client_t`

| Field Name      | Type                 | Description   |
| --------------- | -------------------- | ------------- |
| name            | `char[64]`           | Client name   |
| private_ipv4    | `char[16]`           | VPN IP address |
| public_ipv4     | `char[16]`           | Real IP address |
| since           | `char[32]`           | Connection time |
| bytes_received  | `unsigned long long` | Bytes received |
| bytes_sent      | `unsigned long long` | Bytes sent     |

##### Create Client

```cpp
ovpn_err_t ovpn_mana_create_client(ovpn_mana_handle_t handle, const char *service_name, const char *name, const char* wanip);
```

###### Parameters

| Field Name   | Type                 | Description                    |
| ---------- | -------------------- | ------------------------------ |
| handle     | `ovpn_mana_handle_t` | Manager instance pointer       |
| service_name | `const char *`       | Service name                   |
| name       | `const char*`        | Client name                    |
| wanip      | `const char*`        | Public address accessed by client |

##### Revoke Client

```cpp
ovpn_err_t ovpn_mana_revoke_client(ovpn_mana_handle_t handle, const char *service_name, const char *name);
```

###### Parameters

| Field Name   | Type                 | Description          |
| ---------- | -------------------- | -------------------- |
| handle     | `ovpn_mana_handle_t` | Manager instance pointer |
| service_name | `const char *`       | Service name         |
| name       | `const char *`       | Client name          |

##### Get Client OVPN File Content

```cpp
ovpn_err_t ovpn_mana_get_client_config(ovpn_mana_handle_t handle, const char *service_name, const char *name, char *ovpn_file, int &ovpn_file_size);
```

###### Parameters

| Field Name      | Type                 | Description                                                                  |
| --------------- | -------------------- | ---------------------------------------------------------------------------- |
| handle          | `ovpn_mana_handle_t` | Manager instance pointer                                                     |
| service_name    | `const char *`       | Service name                                                                 |
| name            | `const char*`        | Client name                                                                  |
| ovpn_file       | `char *`             | Content of the client OVPN configuration file, caller allocates pointer, suggested size > 10K |
| ovpn_file_size  | `int&`               | Actual size of the client OVPN configuration file                            |

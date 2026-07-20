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

---

## C# Integration Guide

This section provides complete C# P/Invoke declarations and data structure definitions for quick integration with .NET / ABP applications using `libovpn-mana.so` or `ovpn-mana.dll`.

### Prerequisites

#### Linux (.so)

```csharp
// Deploy libovpn-mana.so to /usr/local/lib/ or application directory
// Set LD_LIBRARY_PATH environment variable or use absolute path in DllImport
```

#### Windows (.dll)

```csharp
// Place ovpn-mana.dll and ovpn-mana.lib in application directory or System32
// Ensure OpenVPN and Easy-RSA are installed and in PATH
```

### Complete C# P/Invoke Definitions

```csharp
using System;
using System.Runtime.InteropServices;

namespace OvpnMana
{
    /// <summary>
    /// OVPN-MANA Native Library P/Invoke Wrapper
    /// Supports Linux (libovpn-mana.so) and Windows (ovpn-mana.dll)
    /// </summary>
    public static class OvpnManaNative
    {
        #region Platform Detection & Library Name

        private const string LinuxLibrary = "libovpn-mana.so";
        private const string WindowsLibrary = "ovpn-mana.dll";

        private static string LibraryName =>
            RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? WindowsLibrary
                : LinuxLibrary;

        #endregion

        #region Lifecycle Management

        /// <summary>
        /// Create manager instance
        /// Returns handle for subsequent API calls
        /// </summary>
        /// <returns>Manager handle</returns>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ovpn_mana_create();

        /// <summary>
        /// Destroy manager instance and release resources
        /// </summary>
        /// <param name="handle">Manager handle</param>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ovpn_mana_destroy(IntPtr handle);

        #endregion

        #region Service Management

        /// <summary>
        /// Get service list (only returns services created via OVPN-MANA)
        /// </summary>
        /// <param name="handle">Manager handle</param>
        /// <param name="services">Service array pointer (caller-allocated)</param>
        /// <param name="service_count">Input: array capacity, Output: actual count</param>
        /// <returns>Error code</returns>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_list_services(
            IntPtr handle,
            [Out] OvpnService[] services,
            ref int service_count);

        /// <summary>
        /// Create OpenVPN service instance
        /// </summary>
        /// <param name="handle">Manager handle</param>
        /// <param name="name">Service name</param>
        /// <param name="subnet">Client subnet CIDR (e.g., 10.8.0.0/24)</param>
        /// <param name="port">Listen port</param>
        /// <returns>Error code</returns>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_create_service(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string subnet,
            int port);

        /// <summary>
        /// Start service
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_start_service(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        /// <summary>
        /// Stop service
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_stop_service(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        /// <summary>
        /// Restart service
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_restart_service(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        /// <summary>
        /// Delete service (includes certificate revocation)
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_delete_service(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        #endregion

        #region Client Management

        /// <summary>
        /// Create client (dynamic VPN IP assignment)
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_create_client(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string service_name,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string wanip);

        /// <summary>
        /// Create client with fixed IP address
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_create_client_with_ip(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string service_name,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string wanip,
            [MarshalAs(UnmanagedType.LPStr)] string client_ip);

        /// <summary>
        /// Revoke client certificate
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_revoke_client(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string service_name,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        /// <summary>
        /// Get online client list (sorted by IP ascending, includes traffic stats)
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_get_online_clients(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string service_name,
            [Out] OvpnClient[] clients,
            ref int client_count);

        /// <summary>
        /// Get total client count (including offline)
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_get_total_clients_count(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string service_name,
            ref int total_count);

        /// <summary>
        /// Get client .ovpn configuration file content
        /// </summary>
        /// <param name="ovpn_file">Buffer pointer (caller-allocated, suggest > 10KB)</param>
        /// <param name="ovpn_file_size">Input: buffer size, Output: actual content size</param>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_get_client_config(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string service_name,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            StringBuilder ovpn_file,
            ref int ovpn_file_size);

        /// <summary>
        /// Export client configuration (alias for above)
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_export_client_config(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string service_name,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            StringBuilder buffer,
            ref int buffer_size);

        #endregion

        #region Configuration & Version

        /// <summary>
        /// Configure runtime settings (override default paths)
        /// </summary>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ovpn_mana_configure(
            IntPtr handle,
            ref OvpnConfig config);

        /// <summary>
        /// Get library version string
        /// </summary>
        /// <returns>Version (e.g., "1.0.0.20626")</returns>
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        public static extern string ovpn_mana_get_version();

        #endregion
    }

    #region Data Structure Definitions

    /// <summary>
    /// Service information structure
    /// Maps to C: ovpn_service_t
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct OvpnService
    {
        /// <summary>Service name (max 64 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string Name;

        /// <summary>Configuration file path (max 256 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string ConfigPath;

        /// <summary>Listen port</summary>
        public int Port;

        /// <summary>Client subnet CIDR (e.g., "10.8.0.0/24", max 32 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string Subnet;

        /// <summary>Is running (1=yes, 0=no)</summary>
        public int IsActivated;

        /// <summary>Auto-start on boot (1=yes, 0=no)</summary>
        public int IsEnabled;
    }

    /// <summary>
    /// Online client information structure
    /// Maps to C: ovpn_client_t
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct OvpnClient
    {
        /// <summary>Client name (max 128 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string Name;

        /// <summary>VPN internal IP (max 32 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string PrivateIpv4;

        /// <summary>Public IP:Port (max 64 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string PublicIpv4;

        /// <summary>Connection time (max 64 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string Since;

        /// <summary>Bytes received</summary>
        public ulong BytesReceived;

        /// <summary>Bytes sent</summary>
        public ulong BytesSent;
    }

    /// <summary>
    /// Runtime configuration structure (custom path overrides)
    /// Maps to C: ovpn_config_t
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct OvpnConfig
    {
        /// <summary>Easy-RSA installation path (max 256 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string EasyRsaDir;

        /// <summary>OpenVPN config root directory (max 256 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string OvpnDir;

        /// <summary>OpenVPN binary full path (max 256 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string OpenvpnBin;

        /// <summary>systemctl binary full path (max 256 chars)</summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string SystemctlBin;
    }

    #endregion

    #region Error Code Definition

    /// <summary>
    /// Error code enumeration
    /// Maps to C: ovpn_err_t and macro definitions
    /// </summary>
    public enum OvpnError : int
    {
        Success = 0,
        Failure = -1,
        InvalidParam = -2,
        NotFound = -3,
        PermissionDenied = -4,
        Timeout = -5,
        IoFailure = -6,
        NameTooLong = -1002,
        IpFormat = -1003,
        PortRange = -1004,
        ForbiddenChar = -1005,
        BufferTooSmall = -1006,
        ServiceNotFound = -1007,
        ClientNotFound = -1008,
        IpConflict = -1009
    }

    #endregion
}
```

### Usage Examples

#### Basic Usage (Raw P/Invoke)

```csharp
using System;
using System.Text;
using OvpnMana;

class Program
{
    static void Main()
    {
        // 1. Create manager instance
        IntPtr handle = OvpnManaNative.ovpn_mana_create();
        try
        {
            // 2. Query version
            string version = OvpnManaNative.ovpn_mana_get_version();
            Console.WriteLine($"OVPN-MANA Version: {version}");

            // 3. List all services
            var services = new OvpnService[16];
            int count = services.Length;
            int err = OvpnManaNative.ovpn_mana_list_services(handle, services, ref count);
            if (err == (int)OvpnError.Success)
            {
                Console.WriteLine($"\n=== Services ({count}) ===");
                for (int i = 0; i < count; i++)
                {
                    Console.WriteLine($"[{i}] {services[i].Name} | " +
                        $"Port:{services[i].Port} | " +
                        $"Subnet:{services[i].Subnet} | " +
                        $"Active:{services[i].IsActivated}");
                }
            }

            // 4. Create new service
            err = OvpnManaNative.ovpn_mana_create_service(handle, "myvpn", "10.8.0.0/24", 1194);
            if (err == (int)OvpnError.Success)
            {
                Console.WriteLine("\nService created successfully.");

                // 5. Start service
                err = OvpnManaNative.ovpn_mana_start_service(handle, "myvpn");
                if (err == (int)OvpnError.Success)
                    Console.WriteLine("Service started.");
            }
            else
            {
                Console.WriteLine($"Create service failed: {(OvpnError)err}");
            }

            // 6. Create client
            err = OvpnManaNative.ovpn_mana_create_client(handle, "myvpn", "alice", "1.2.3.4");
            if (err == (int)OvpnError.Success)
                Console.WriteLine("Client 'alice' created.");

            // 7. Get online clients
            var clients = new OvpnClient[64];
            int clientCount = clients.Length;
            err = OvpnManaNative.ovpn_mana_get_online_clients(handle, "myvpn", clients, ref clientCount);
            if (err == (int)OvpnError.Success && clientCount > 0)
            {
                Console.WriteLine($"\n=== Online Clients ({clientCount}) ===");
                for (int i = 0; i < clientCount; i++)
                {
                    Console.WriteLine($"- {clients[i].Name} | " +
                        $"VPN:{clients[i].PrivateIpv4} | " +
                        $"Public:{clients[i].PublicIpv4} | " +
                        $"RX:{FormatBytes(clients[i].BytesReceived)} | " +
                        $"TX:{FormatBytes(clients[i].BytesSent)}");
                }
            }

            // 8. Export client config file
            var buffer = new StringBuilder(16384); // 16KB buffer
            int bufferSize = buffer.Capacity;
            err = OvpnManaNative.ovpn_mana_get_client_config(handle, "myvpn", "alice", buffer, ref bufferSize);
            if (err == (int)OvpnError.Success)
            {
                Console.WriteLine($"\n=== Alice's OVPN Config ({bufferSize} bytes) ===");
                Console.WriteLine(buffer.ToString());
            }
        }
        finally
        {
            // 9. Destroy manager (release resources)
            OvpnManaNative.ovpn_mana_destroy(handle);
        }
    }

    static string FormatBytes(ulong bytes)
    {
        string[] sizes = { "B", "KB", "MB", "GB", "TB" };
        double len = bytes;
        int order = 0;
        while (len >= 1024 && order < sizes.Length - 1)
        {
            order++;
            len /= 1024;
        }
        return $"{len:0.##} {sizes[order]}";
    }
}
```

#### ABP vNext Application Service Wrapper (Recommended)

```csharp
using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using OvpnMana;
using Volo.Abp;
using Volo.Abp.Application.Services;

namespace MyProject.Vpn
{
    /// <summary>
    /// VPN Management Application Service (ABP vNext style)
    /// Provides async wrapping and exception translation
    /// </summary>
    public class VpnManagementAppService : ApplicationService, IVpnManagementAppService
    {
        private readonly ILogger<VpnManagementAppService> _logger;

        public VpnManagementAppService(ILogger<VpnManagementAppService> logger)
        {
            _logger = logger;
        }

        private IntPtr EnsureHandle()
        {
            var handle = OvpnManaNative.ovpn_mana_create();
            if (handle == IntPtr.Zero)
                throw new UserFriendlyException("Failed to initialize OVPN-MANA manager");
            return handle;
        }

        private void ThrowIfError(int errorCode, string operation)
        {
            if (errorCode != (int)OvpnError.Success)
            {
                _logger.LogError("{Operation} failed with error code: {ErrorCode}", operation, errorCode);
                throw new UserFriendlyException($"{operation} failed: {(OvpnError)errorCode}");
            }
        }

        public async Task<List<ServiceDto>> GetServicesAsync()
        {
            await Task.CompletedTask; // P/Invoke is synchronous, compatible with ABP async pattern
            var handle = EnsureHandle();
            try
            {
                var services = new OvpnService[32];
                int count = services.Length;
                int err = OvpnManaNative.ovpn_mana_list_services(handle, services, ref count);
                ThrowIfError(err, "Get service list");

                var result = new List<ServiceDto>(count);
                for (int i = 0; i < count; i++)
                {
                    result.Add(new ServiceDto
                    {
                        Name = services[i].Name,
                        Port = services[i].Port,
                        Subnet = services[i].Subnet,
                        IsActivated = services[i].IsActivated == 1,
                        IsEnabled = services[i].IsEnabled == 1
                    });
                }
                return result;
            }
            finally
            {
                OvpnManaNative.ovpn_mana_destroy(handle);
            }
        }

        public async Task CreateServiceAsync(string name, string subnet, int port)
        {
            await Task.CompletedTask;
            var handle = EnsureHandle();
            try
            {
                int err = OvpnManaNative.ovpn_mana_create_service(handle, name, subnet, port);
                ThrowIfError(err, $"Create service '{name}'");

                _logger.LogInformation("VPN service created: {Name}, Port: {Port}, Subnet: {Subnet}",
                    name, port, subnet);
            }
            finally
            {
                OvpnManaNative.ovpn_mana_destroy(handle);
            }
        }

        public async Task CreateClientAsync(string serviceName, string clientName, string wanIp, string fixedIp = null)
        {
            await Task.CompletedTask;
            var handle = EnsureHandle();
            try
            {
                int err;
                if (!string.IsNullOrWhiteSpace(fixedIp))
                {
                    err = OvpnManaNative.ovpn_mana_create_client_with_ip(
                        handle, serviceName, clientName, wanIp, fixedIp);
                }
                else
                {
                    err = OvpnManaNative.ovpn_mana_create_client(
                        handle, serviceName, clientName, wanIp);
                }
                ThrowIfError(err, $"Create client '{clientName}'");

                _logger.LogInformation("VPN client created: {ClientName}@{Service}", clientName, serviceName);
            }
            finally
            {
                OvpnManaNative.ovpn_mana_destroy(handle);
            }
        }

        public async Task<List<ClientDto>> GetOnlineClientsAsync(string serviceName)
        {
            await Task.CompletedTask;
            var handle = EnsureHandle();
            try
            {
                var clients = new OvpnClient[128];
                int count = clients.Length;
                int err = OvpnManaNative.ovpn_mana_get_online_clients(handle, serviceName, clients, ref count);
                ThrowIfError(err, $"Get online clients for '{serviceName}'");

                var result = new List<ClientDto>(count);
                for (int i = 0; i < count; i++)
                {
                    result.Add(new ClientDto
                    {
                        Name = clients[i].Name,
                        PrivateIpv4 = clients[i].PrivateIpv4,
                        PublicIpv4 = clients[i].PublicIpv4,
                        Since = clients[i].Since,
                        BytesReceived = clients[i].BytesReceived,
                        BytesSent = clients[i].BytesSent
                    });
                }
                return result;
            }
            finally
            {
                OvpnManaNative.ovpn_mana_destroy(handle);
            }
        }

        public async Task<string> ExportClientConfigAsync(string serviceName, string clientName)
        {
            await Task.CompletedTask;
            var handle = EnsureHandle();
            try
            {
                var buffer = new StringBuilder(32768); // 32KB
                int size = buffer.Capacity;
                int err = OvpnManaNative.ovpn_mana_get_client_config(
                    handle, serviceName, clientName, buffer, ref size);
                ThrowIfError(err, $"Export config for '{clientName}'");

                return buffer.ToString(0, size);
            }
            finally
            {
                OvpnManaNative.ovpn_mana_destroy(handle);
            }
        }

        public async Task RevokeClientAsync(string serviceName, string clientName)
        {
            await Task.CompletedTask;
            var handle = EnsureHandle();
            try
            {
                int err = OvpnManaNative.ovpn_mana_revoke_client(handle, serviceName, clientName);
                ThrowIfError(err, $"Revoke client '{clientName}'");

                _logger.LogWarning("VPN client revoked: {ClientName}@{Service}", clientName, serviceName);
            }
            finally
            {
                OvpnManaNative.ovpn_mana_destroy(handle);
            }
        }
    }

    #region DTOs

    public class ServiceDto
    {
        public string Name { get; set; }
        public int Port { get; set; }
        public string Subnet { get; set; }
        public bool IsActivated { get; set; }
        public bool IsEnabled { get; set; }
    }

    public class ClientDto
    {
        public string Name { get; set; }
        public string PrivateIpv4 { get; set; }
        public string PublicIpv4 { get; set; }
        public string Since { get; set; }
        public ulong BytesReceived { get; set; }
        public ulong BytesSent { get; set; }
    }

    public interface IVpnManagementAppService : IApplicationService
    {
        Task<List<ServiceDto>> GetServicesAsync();
        Task CreateServiceAsync(string name, string subnet, int port);
        Task CreateClientAsync(string serviceName, string clientName, string wanIp, string fixedIp = null);
        Task<List<ClientDto>> GetOnlineClientsAsync(string serviceName);
        Task<string> ExportClientConfigAsync(string serviceName, string clientName);
        Task RevokeClientAsync(string serviceName, string clientName);
    }

    #endregion
}
```

### Important Notes for C# Developers

1. **Platform Differences**
   - Linux uses `libovpn-mana.so`, Windows uses `ovpn-mana.dll`
   - On Windows, ensure DLL dependencies (VC++ runtime) are installed

2. **Memory Management**
   - Handle returned by `ovpn_mana_create()` must be released via `ovpn_mana_destroy()` after use
   - Use `try-finally` or `IDisposable` wrapper to ensure resource cleanup

3. **Array Buffers**
   - `services` and `clients` arrays must be pre-allocated by the caller
   - Pass array capacity first, API returns actual element count in `*_count`
   - Recommend allocating large initial space (e.g., 32/128 elements)

4. **String Encoding**
   - All strings use ANSI encoding (`CharSet.Ansi`)
   - Use `[MarshalAs(UnmanagedType.LPStr)]` on C# side for automatic conversion

5. **Thread Safety**
   - Each `handle` is not guaranteed thread-safe
   - In multi-threaded scenarios, create independent handles per thread or add external synchronization locks

6. **Error Handling**
   - All APIs return `int` error codes, must check return values
   - Use `OvpnError` enum for readable error checking

7. **Performance Optimization**
   - For high-frequency call scenarios, consider caching the `handle` (singleton pattern)
   - Reuse the same handle for batch operations to avoid repeated create/destroy overhead
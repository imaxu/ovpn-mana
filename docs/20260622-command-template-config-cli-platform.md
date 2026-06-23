# 20260622 - OpenVPNManager 重构规划

## 一、重构目标

### 1.1 当前问题分析

#### 代码质量问题

- [OpenVPNManager.cpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/OpenVPNManager.cpp) 中存在大量命令行字符串拼接（`"cd " + EASY_RSA_DIR + " && ./easyrsa..."`），散落各处，可读性差、易出错
- 命令模板散落在 372 行代码中，修改一处命令需全局搜索替换，维护成本高

#### 配置灵活性问题

- `EASY_RSA_DIR`、`OVPN_DIR` 等路径通过 [CMakeLists.txt](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/CMakeLists.txt) 编译期写死，换机器部署必须重新编译
- 无法在运行时为不同 handle 注入不同路径，不支持多实例场景

#### 功能缺失

- 缺少客户端 `.ovpn` 配置文件导出功能，用户无法通过 SDK 获取服务端生成的配置文件
- CCD 固定 IP 功能不完整：`createService` 虽创建了 `ccd/` 目录和 `client-config-dir` 指令，但 `createClient` 从未写入 `ifconfig-push` 文件

#### 工程规范问题

- 版本号硬编码为 `1.0.0.1`，修订号需人工维护，缺乏可追溯性
- [main.cpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/main.cpp) 控制台输出使用 `=====` 分隔线 + `\t\t` 缩进，客户端列表为单行长文本，87 个客户端时几乎不可读
- 跨平台处理（`#ifdef _WIN32`）散落在多个文件中，缺乏统一抽象层，新增平台时需逐个文件修改

### 1.2 需求

| 编号 | 需求                                                                             | 优先级 | 关联章节                                  |
| ---- | -------------------------------------------------------------------------------- | ------ | ----------------------------------------- |
| R1   | 命令模板对象化：将散落的字符串拼接重构为集中式 `constexpr` 模板 + 参数替换     | P0     | [二.2.1](#21-命令模板对象化重构)             |
| R2   | 运行时配置注入：`ovpn_mana_configure()` 替代编译期 `config.hpp`，支持多实例  | P0     | [二.2.4](#24-运行时配置注入替代编译期硬编码) |
| R3   | 客户端配置导出：`ovpn_mana_export_client_config()` 两阶段获取 `.ovpn` 文件流 | P0     | [二.2.3](#23-新增下载客户端配置导出函数)     |
| R4   | CCD 固定 IP 支持：`createClient` 写入 `ifconfig-push`，含 IP 重复性校验      | P1     | [九](#九缺口分析客户端固定-ipccd-支持)       |
| R5   | 版本管理：4 段版本号 `MAJOR.MINOR.PATCH.DAYS`，第 4 段为 UTC 1970 累计天数     | P1     | [十二](#十二版本管理方案)                    |
| R6   | CLI 输出优化：ASCII 框线 Banner + 表格化客户端列表 + 人类可读字节数 + 颜色       | P2     | [十三](#十三maincpp-输出格式优化)            |
| R7   | 跨平台兼容：统一 `platform.hpp` 抽象层，`std::filesystem` 跨平台路径         | P1     | [十四](#十四跨平台兼容性保障)                |
| R8   | 兼容性回归：确保所有变更向后兼容，旧调用方仅需最小适配                           | P0     | [十](#十兼容性回归审查)                      |
| R9   | 参数校验：名称/固定IP格式校验，防止命令注入和脏数据落盘                          | P0     | [十五](#十五参数校验方案)                    |

### 1.3 重构原则

1. **对外接口不变，内部实现可换**：C API 尽可能保持签名不变，仅在必要时末尾追加可选参数
2. **不调用新 API 则行为与旧版完全一致**：`ovpn_mana_configure()` 不调用时使用默认值，`client_ip` 不传时自动分配
3. **先规划后实施**：本文档为实施蓝图，所有代码改动前需评审本文档中的方案
4. **关键路径有测试覆盖**：命令模板替换、配置注入、IP 冲突检测、导出功能均需单元测试

---

## 二、重构方案

### 2.1 命令模板对象化重构

#### 方案选型

| 方案                       | 优点             | 缺点                     | 选择        |
| -------------------------- | ---------------- | ------------------------ | ----------- |
| 宏定义拼接                 | 简洁，编译期展开 | 难以调试，不支持复杂逻辑 | ❌          |
| constexpr std::string 模板 | 类型安全，可调试 | C++17 支持良好           | ⭐          |
| CommandBuilder 类          | 灵活，可扩展     | 少量运行时开销           | ✅ 结合使用 |

**最终选型**：`constexpr` 字符串模板 + 参数替换工具函数

#### 2.1.1 新增 `CommandTemplates.hpp`

```
include/
└── CommandTemplates.hpp  # 新增 - 所有命令模板集中管理
```

核心设计：

```cpp
#pragma once
#include <string>
#include <string_view>

namespace ovpn::commands {

// 占位符约定：{cfg.EASY_RSA_DIR} 等路径占位符由 replace() 函数自动从运行时配置注入
// 调用方只需提供业务参数（如 NAME、PORT 等），路径参数无需手动传入

// easy-rsa 命令模板
constexpr std::string_view EASYRSA_GEN_REQ =
    "cd {cfg.EASY_RSA_DIR} && ./easyrsa --batch gen-req {NAME} nopass";

constexpr std::string_view EASYRSA_SIGN_REQ_SERVER =
    "cd {cfg.EASY_RSA_DIR} && ./easyrsa --batch sign-req server {NAME}";

constexpr std::string_view EASYRSA_BUILD_CLIENT_FULL =
    "cd {cfg.EASY_RSA_DIR} && ./easyrsa --batch build-client-full {NAME} nopass";

constexpr std::string_view EASYRSA_REVOKE =
    "cd {cfg.EASY_RSA_DIR} && ./easyrsa --batch revoke {NAME}";

constexpr std::string_view EASYRSA_GEN_CRL =
    "cd {cfg.EASY_RSA_DIR} && ./easyrsa gen-crl";

constexpr std::string_view EASYRSA_GEN_DH =
    "cd {cfg.EASY_RSA_DIR} && ./easyrsa gen-dh";

// systemctl 命令模板
constexpr std::string_view SYSTEMCTL_START =
    "{cfg.SYSTEMCTL_BIN} start openvpn@{NAME}-server";

constexpr std::string_view SYSTEMCTL_STOP =
    "sudo {cfg.SYSTEMCTL_BIN} stop openvpn@{NAME}-server";

constexpr std::string_view SYSTEMCTL_RESTART =
    "{cfg.SYSTEMCTL_BIN} restart openvpn@{NAME}-server";

constexpr std::string_view SYSTEMCTL_ENABLE =
    "{cfg.SYSTEMCTL_BIN} enable openvpn@{NAME}-server";

constexpr std::string_view SYSTEMCTL_DISABLE =
    "{cfg.SYSTEMCTL_BIN} disable openvpn@{NAME}-server";

constexpr std::string_view SYSTEMCTL_IS_ACTIVE =
    "{cfg.SYSTEMCTL_BIN} is-active openvpn@{NAME}-server";

constexpr std::string_view SYSTEMCTL_IS_ENABLED =
    "{cfg.SYSTEMCTL_BIN} is-enabled openvpn@{NAME}-server";

// openvpn 命令模板
constexpr std::string_view OPENVPN_GEN_TA_KEY =
    "sudo {cfg.OPENVPN_BIN} --genkey secret {OUTPUT_PATH}";

// 文件操作命令
constexpr std::string_view CP_WITH_SUDO =
    "sudo cp {SRC} {DEST}";

constexpr std::string_view CHMOD =
    "sudo chmod {MODE} {PATH}";

constexpr std::string_view MV_WITH_SUDO =
    "sudo mv {SRC} {DEST}";

// 参数替换工具函数
// 自动将 {cfg.XXX} 占位符替换为运行时配置对象中的对应字段
std::string replace(const std::string_view& tmpl,
                    const AppConfig& cfg,
                    const std::initializer_list<std::pair<std::string_view, std::string>>& params = {});

} // namespace ovpn::commands
```

#### 2.1.2 `CommandTemplates.cpp` 实现参数替换

```cpp
#include "CommandTemplates.hpp"
#include <algorithm>

namespace ovpn::commands {

std::string replace(const std::string_view& tmpl,
                    const AppConfig& cfg,
                    const std::initializer_list<std::pair<std::string_view, std::string>>& params)
{
    std::string result{tmpl};

    // 优先替换运行时配置中的键（cfg.* 占位符自动映射）
    static const std::map<std::string_view, std::string AppConfig::*> cfgMap = {
        {"cfg.EASY_RSA_DIR", &AppConfig::easyRsaDir},
        {"cfg.OVPN_DIR", &AppConfig::ovpnDir},
        {"cfg.OPENVPN_BIN", &AppConfig::openvpnBin},
        {"cfg.SYSTEMCTL_BIN", &AppConfig::systemctlBin},
        {"cfg.OVPN_SERVER_CONF_DIR", &AppConfig::ovpnServerConfDir},
    };
    for (const auto& [key, member] : cfgMap) {
        std::string placeholder = "{" + std::string{key} + "}";
        const std::string& value = cfg.*member;
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    // 替换用户传入的业务参数
    for (const auto& [key, value] : params) {
        std::string placeholder = "{" + std::string{key} + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    return result;
}

} // namespace ovpn::commands
```

#### 2.1.3 改造前后对比

**改造前**：

```cpp
std::string cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa --batch gen-req " + name + "-server nopass";
std::string output;
if (!execCommand(cmd, output))
    return false;
```

**改造后**：

```cpp
std::string cmd = commands::replace(commands::EASYRSA_GEN_REQ, cfg, {
    {"NAME", name + "-server"}
});
std::string output;
if (!execCommand(cmd, output))
    return false;
```

**收益**：

- 命令模板集中管理，便于全局修改
- 占位符语义清晰，减少拼写错误
- 路径参数由运行时配置自动注入，无需在每个调用点手动传入
- 代码行数减少，可读性提升

---

### 2.2 配置模板提取

当前服务端配置和客户端配置都是内联 `std::ostringstream` 拼接，提取为配置模板：

在 `CommandTemplates.hpp` 中增加：

```cpp
namespace ovpn::templates {

// 服务端配置模板
constexpr std::string_view SERVER_CONFIG = R"(
topology subnet
port {PORT}
proto udp
dev tun
ca {CA_PATH}
cert {CERT_PATH}
key {KEY_PATH}
dh {DH_PATH}
tls-auth {TA_PATH} 0
server {SUBNET} 255.255.255.0
keepalive 10 120
persist-key
persist-tun
ifconfig-pool-persist {IPP_TXT_PATH}
client-config-dir {CCD_DIR}
status {STATUS_LOG_PATH}
verb 3
)";

// 客户端配置模板
constexpr std::string_view CLIENT_CONFIG = R"(
client
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
<ca>
{CA_CONTENT}
</ca>
<cert>
{CERT_CONTENT}
</cert>
<key>
{KEY_CONTENT}
</key>
tls-auth {TA_CONTENT}
key-direction 1
)";

} // namespace ovpn::templates
```

使用方式：

```cpp
std::string config = templates::replace(templates::SERVER_CONFIG, {
    {"PORT", std::to_string(port)},
    {"SUBNET", subnet},
    {"CA_PATH", OVPN_SERVER_CONF_DIR + "/" + name + "/ca.crt"},
    // ... 其他占位符
});
```

---

### 2.3 新增下载客户端配置导出函数

#### 2.3.1 需求分析

- 用户侧创建客户端成功后，需要获取服务端生成的 `.ovpn` 文件流
- 保存到用户侧本地文件
- 需要在 C API 层导出，支持跨语言调用

#### 2.3.2 接口设计

**第一步**：在 [sdk.types.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/sdk.types.hpp) 中无变更，复用现有 `ovpn_err_t` 错误码机制。

**第二步**：在 [ovpn-mana.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/ovpn-mana.hpp) 中新增导出声明：

```cpp
/// @brief  导出客户端配置文件内容（用于下载）
/// @param  handle  句柄
/// @param  service_name  服务名称
/// @param  client_name  客户端名称
/// @param  buffer  输出缓冲区，用于接收配置内容
/// @param  buffer_size  传入时为缓冲区大小，传出时为实际内容大小
/// @return  错误码
/// @note    两阶段调用：先传 buffer=nullptr 获取所需大小，分配内存后再调用
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_export_client_config(
    ovpn_mana_handle_t handle,
    const char *service_name,
    const char *client_name,
    char *buffer,
    int &buffer_size);
```

**第三步**：在 [OpenVPNManager.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/OpenVPNManager.hpp) 已经存在 `getOVPNFileContent`，只需要确保接口稳定。

当前已有实现：

```cpp
static std::string getOVPNFileContent(const std::string &name, const std::string &serviceName);
```

该方法已经满足需求，返回完整配置内容字符串。

**第四步**：在 [ovpn-mana.cpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/ovpn-mana.cpp) 中新增 C API 实现：

```cpp
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_export_client_config(
    ovpn_mana_handle_t handle,
    const char *service_name,
    const char *client_name,
    char *buffer,
    int &buffer_size)
{
    try {
        auto *mgr = reinterpret_cast<OpenVPNManager*>(handle);
        std::string content = mgr->getOVPNFileContent(client_name, service_name);

        if (buffer == nullptr) {
            // 第一阶段调用：返回所需大小
            buffer_size = static_cast<int>(content.size()) + 1; // +1 用于 null 终止符
            return OVPN_ERR_SUCCESS;
        }

        if (buffer_size < static_cast<int>(content.size()) + 1) {
            // 缓冲区不足
            buffer_size = static_cast<int>(content.size()) + 1;
            return OVPN_ERR_FAILURE; // 或者新增 OVPN_ERR_BUFFER_TOO_SMALL
        }

        // 复制内容到调用方缓冲区
        std::strncpy(buffer, content.c_str(), content.size() + 1);
        buffer_size = static_cast<int>(content.size());
        return OVPN_ERR_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Failed to export client config: " << e.what() << std::endl;
        return OVPN_ERR_FAILURE;
    }
}
```

**第五步**：在 [main.cpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/main.cpp) 中新增 CLI 子命令支持：

```cpp
// 在 client 命令分支增加
else if (sub_command == "-export") {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " client -export <service_name>,<client_name>\n";
        ovpn_mana_destroy(handle);
        return -1;
    }
    std::string client_info = argv[3];
    size_t comma_pos = client_info.find(',');
    if (comma_pos == std::string::npos) {
        std::cerr << "Invalid format. Use <service_name>,<client_name>\n";
        ovpn_mana_destroy(handle);
        return -1;
    }
    std::string service_name = client_info.substr(0, comma_pos);
    std::string client_name = client_info.substr(comma_pos + 1);

    int size = 0;
    ovpn_err_t err = ovpn_mana_export_client_config(handle,
        service_name.c_str(), client_name.c_str(), nullptr, size);
    if (err != OVPN_ERR_SUCCESS) {
        std::cerr << "Failed to get config size\n";
        ovpn_mana_destroy(handle);
        return -1;
    }

    char *buf = new char[size];
    err = ovpn_mana_export_client_config(handle,
        service_name.c_str(), client_name.c_str(), buf, size);
    if (err != OVPN_ERR_SUCCESS) {
        std::cerr << "Failed to export client config\n";
        delete[] buf;
        ovpn_mana_destroy(handle);
        return -1;
    }

    std::cout.write(buf, size);
    delete[] buf;
    ovpn_mana_destroy(handle);
    return 0;
}
```

更新帮助信息，增加：

```
  client  -export <service_name>,<client_name>  Export client config to stdout (for download)
```

---

### 2.4 运行时配置注入（替代编译期硬编码）

#### 2.4.1 现状分析

当前配置路径在 CMake 编译期写死：

```
CMakeLists.txt  ──configure_file()──▶  config.hpp  ──#include──▶  OpenVPNManager.cpp
     │                                      │
     │  set(EASY_RSA_DIR "...")             │  const std::string EASY_RSA_DIR = "/home/xuwh/easy-rsa";
     │  set(OVPN_DIR "...")                 │  const std::string OVPN_DIR = "/etc/openvpn";
     └──────────────────────────────────────┘
```

**问题**：

- 换一台机器部署需要重新编译（写死路径 `/home/xuwh/easy-rsa`）
- 同一个 SDK 无法同时管理不同路径下的多个 easy-rsa 实例
- 测试环境和生产环境路径不同时，需要编译两套 `.so`

#### 2.4.2 方案：新增 `AppConfig` 结构体 + `configure()` 注入函数

**核心思路**：`OpenVPNManager` 从静态方法改为持有 `AppConfig` 实例，由调用方在 `ovpn_mana_create()` 之后通过 `ovpn_mana_configure()` 注入路径。

```
调用方（Python / Node.js / CLI）
  │
  │  ovpn_mana_create()          → 返回 handle
  │  ovpn_mana_configure(handle, &cfg)  → 注入运行时路径
  │  ovpn_mana_create_service(handle, ...)  → 使用注入的路径
  │
  └──────────────────────────────────────────────
```

#### 2.4.3 数据结构设计

**第一步**：在 [sdk.types.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/sdk.types.hpp) 新增 `ovpn_config_t`：

```cpp
typedef struct {
    char easy_rsa_dir[256];      // easy-rsa 安装目录，如 "/home/user/easy-rsa"
    char ovpn_dir[256];          // OpenVPN 配置目录，如 "/etc/openvpn"
    char openvpn_bin[256];       // openvpn 可执行文件路径，如 "/usr/sbin/openvpn"
    char systemctl_bin[256];     // systemctl 可执行文件路径，如 "/bin/systemctl"
} ovpn_config_t;
```

**第二步**：在 [OpenVPNManager.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/OpenVPNManager.hpp) 中新增 `AppConfig` 和 `configure()`：

```cpp
struct AppConfig {
    std::string easyRsaDir;
    std::string ovpnDir;
    std::string openvpnBin;
    std::string systemctlBin;

    // 派生路径（自动计算，调用方无需手动填入）
    std::string ovpnServerConfDir() const { return ovpnDir + "/server"; }
    std::string clientConfigsDir()  const { return ovpnDir + "/client-configs"; }

    // 提供默认值，确保不配置也能运行（兼容旧行为）
    static AppConfig defaults() {
        return AppConfig{
            "/home/xuwh/easy-rsa",
            "/etc/openvpn",
            "/usr/sbin/openvpn",
            "/bin/systemctl"
        };
    }
};

class OpenVPNManager {
public:
    // 注入运行时配置（必须在 createService 等操作前调用）
    void configure(const AppConfig& cfg) { m_config = cfg; }
    const AppConfig& config() const { return m_config; }

    // 所有方法从 static 改为非 static 成员方法，通过 m_config 获取路径
    std::vector<VPNService> listServices();
    bool createService(const std::string &name, const std::string &subnet, int port = 1194);
    // ... 其余方法签名不变，去掉 static

private:
    AppConfig m_config{AppConfig::defaults()};  // 默认值保证向后兼容
    // ... 其余私有成员
};
```

**第三步**：在 [ovpn-mana.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/ovpn-mana.hpp) 新增 C API 声明：

```cpp
/// @brief  配置 OpenVPN 管理器运行时参数
/// @param  handle  句柄
/// @param  config  配置参数结构体指针
/// @return  错误码
/// @note    必须在调用其他操作 API 之前调用。未调用时使用编译期默认值。
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_configure(
    ovpn_mana_handle_t handle,
    const ovpn_config_t *config);
```

**第四步**：在 [ovpn-mana.cpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/ovpn-mana.cpp) 实现：

```cpp
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_configure(
    ovpn_mana_handle_t handle,
    const ovpn_config_t *config)
{
    try {
        if (!handle || !config) return OVPN_ERR_INVALID_PARAM;

        auto *mgr = reinterpret_cast<OpenVPNManager*>(handle);
        AppConfig cfg;
        cfg.easyRsaDir    = config->easy_rsa_dir;
        cfg.ovpnDir       = config->ovpn_dir;
        cfg.openvpnBin    = config->openvpn_bin[0] ? config->openvpn_bin : "/usr/sbin/openvpn";
        cfg.systemctlBin  = config->systemctl_bin[0] ? config->systemctl_bin : "/bin/systemctl";
        mgr->configure(cfg);
        return OVPN_ERR_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Failed to configure: " << e.what() << std::endl;
        return OVPN_ERR_FAILURE;
    }
}
```

#### 2.4.4 `config.hpp` 重构

`config.hpp` 从编译期常量容器变为**默认值定义文件**：

```cpp
// config.hpp（重构后）
#pragma once
#include <string>

// 默认配置值 —— 仅作为 ovpn_mana_configure() 未调用时的回退
// 调用方应使用 ovpn_mana_configure() 在运行时注入实际路径
namespace ovpn::defaults {
    constexpr const char* EASY_RSA_DIR    = "/home/xuwh/easy-rsa";
    constexpr const char* OVPN_DIR        = "/etc/openvpn";
    constexpr const char* OPENVPN_BIN     = "/usr/sbin/openvpn";
    constexpr const char* SYSTEMCTL_BIN   = "/bin/systemctl";
}
```

`config.hpp.in` 模板保留，但 CMake 变量仅用于修改默认值：

```cpp
// config.hpp.in（重构后）
#pragma once
#include <string>

namespace ovpn::defaults {
    constexpr const char* EASY_RSA_DIR    = "@EASY_RSA_DIR@";
    constexpr const char* OVPN_DIR        = "@OVPN_DIR@";
    constexpr const char* OPENVPN_BIN     = "/usr/sbin/openvpn";
    constexpr const char* SYSTEMCTL_BIN   = "/bin/systemctl";
}
```

#### 2.4.5 改造前后对比

| 维度       | 改造前（编译期）                     | 改造后（运行时注入）              |
| ---------- | ------------------------------------ | --------------------------------- |
| 路径来源   | CMake 变量 →`config.hpp` 全局常量 | `ovpn_mana_configure()` 传入    |
| 部署灵活性 | 换机器需重新编译                     | 同一 `.so` 可在不同环境运行     |
| 多实例     | 不支持                               | 不同 handle 可注入不同路径        |
| 测试友好度 | 需 mock 全局变量                     | 直接在测试中注入测试路径          |
| 向后兼容   | -                                    | 不调用 `configure` 时使用默认值 |

#### 2.4.6 CLI 工具适配

CLI 工具增加 `--easy-rsa-dir` 和 `--ovpn-dir` 参数：

```cpp
// main.cpp 新增参数解析
// ./openvpnmgr --easy-rsa-dir /opt/easy-rsa --ovpn-dir /etc/openvpn service -l

ovpn_config_t cfg = {};
// 从命令行参数填充 cfg，或使用默认值
ovpn_mana_configure(handle, &cfg);
```

#### 2.4.7 调用方示例（Python）

```python
import ctypes

lib = ctypes.CDLL("./libovpn-mana.so")

class ovpn_config_t(ctypes.Structure):
    _fields_ = [
        ("easy_rsa_dir", ctypes.c_char * 256),
        ("ovpn_dir",     ctypes.c_char * 256),
        ("openvpn_bin",  ctypes.c_char * 256),
        ("systemctl_bin", ctypes.c_char * 256),
    ]

handle = lib.ovpn_mana_create()

# 运行时注入配置
cfg = ovpn_config_t()
cfg.easy_rsa_dir = b"/opt/easy-rsa"
cfg.ovpn_dir     = b"/etc/openvpn"
cfg.openvpn_bin  = b"/usr/sbin/openvpn"
lib.ovpn_mana_configure(handle, ctypes.byref(cfg))

# 后续操作使用注入的路径
lib.ovpn_mana_create_service(handle, b"mysrv", b"10.8.0.0", 1194)
```

---

### 2.5 错误码扩展（可选）

如果需要更精确的错误提示，在 [sdk.types.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/sdk.types.hpp) 新增：

```cpp
#define OVPN_ERR_BUFFER_TOO_SMALL  -7
```

---

## 三、目录结构变化

```
sdk-gitee/
├── include/
│   ├── CommandTemplates.hpp      # 新增 - 命令模板和配置模板
│   ├── config.hpp                # 重构 - 从全局常量改为 ovpn::defaults 命名空间
│   ├── config.hpp.in             # 重构 - 仅用于注入默认值
│   ├── OpenVPNManager.hpp        # 修改 - 新增 AppConfig 结构体 + configure()
│   ├── ovpn-mana.hpp             # 修改 - 新增 ovpn_mana_configure() + ovpn_mana_export_client_config()
│   └── sdk.types.hpp             # 修改 - 新增 ovpn_config_t 结构体
├── src/
│   ├── CommandTemplates.cpp      # 新增 - 参数替换实现
│   ├── OpenVPNManager.cpp        # 修改 - 替换字符串拼接为模板替换，static → 成员方法
│   ├── ovpn-mana.cpp             # 修改 - 新增 configure + export 两个 C API 实现
│   └── main.cpp                  # 修改 - 新增 --easy-rsa-dir/--ovpn-dir 参数 + client -export 子命令
├── test/
│   └── test_client.cpp           # 可选新增测试用例
└── docs/
    ├── 开发指南.md
    └── 20260622-重构.md          # 本文档
```

---

## 四、改造步骤

### Step 1: 创建 `CommandTemplates.hpp` 和 `CommandTemplates.cpp`

- [ ] 定义所有命令模板 `constexpr std::string_view`
- [ ] 定义配置模板（服务端/客户端）
- [ ] 实现 `replace` 参数替换函数（含 `cfg.*` 自动映射）
- [ ] 更新 `CMakeLists.txt` 添加新文件到编译目标

### Step 2: 新增运行时配置注入机制

- [ ] 在 `sdk.types.hpp` 新增 `ovpn_config_t` 结构体
- [ ] 在 `OpenVPNManager.hpp` 新增 `AppConfig` 结构体 + `configure()` 方法
- [ ] 在 `OpenVPNManager.hpp` 将所有方法从 `static` 改为非静态成员方法
- [ ] 在 `OpenVPNManager.cpp` 所有路径引用改为 `m_config.xxx`
- [ ] 重构 `config.hpp` / `config.hpp.in` 为 `ovpn::defaults` 命名空间
- [ ] 在 `ovpn-mana.hpp` 新增 `ovpn_mana_configure()` 声明
- [ ] 在 `ovpn-mana.cpp` 实现 `ovpn_mana_configure()`
- [ ] 更新 `CMakeLists.txt`：`configure_file` 逻辑保持不变

### Step 3: 重构 `OpenVPNManager.cpp` 命令拼接

- [ ] 替换 `createService` 中所有命令拼接 → `commands::replace(tmpl, m_config, {...})`
- [ ] 替换 `deleteService` 中所有命令拼接
- [ ] 替换 `createClient` 中所有命令拼接
- [ ] 替换 `revokeClient` 中所有命令拼接
- [ ] 替换服务端配置生成使用模板
- [ ] 替换客户端配置生成使用模板
- [ ] 编译检查，修复错误

### Step 4: C API 层新增导出函数

- [ ] 在 `ovpn-mana.hpp` 添加 `ovpn_mana_export_client_config` 声明
- [ ] 在 `ovpn-mana.cpp` 实现 `ovpn_mana_export_client_config`
- [ ] 验证导出符号（Linux 下 `nm libovpn-mana.so` 检查）

### Step 5: CLI 层增加配置参数和导出命令

- [ ] 在 `main.cpp` 解析 `--easy-rsa-dir` / `--ovpn-dir` 参数
- [ ] 在 `main.cpp` 解析 `client -export` 子命令
- [ ] 实现两阶段获取输出，写入标准输出
- [ ] 更新帮助文本

### Step 6: 版本管理设置

- [ ] 创建 `include/version.hpp.in` 模板
- [ ] 修改 `CMakeLists.txt`：自动计算 `PROJECT_VERSION_DAYS`
- [ ] 创建 `version.rc.in`（Windows 版本资源）
- [ ] 在 `main.cpp` 新增 `--version` / `-V` 参数

### Step 7: main.cpp 输出格式优化

- [ ] 新增 `printBanner()` 函数（框线 + 版本号 + 版权）
- [ ] 新增 `formatBytes()` / `truncate()` / `printTableHeader()` 等格式化函数
- [ ] 替换 `client -l` 输出为表格化输出
- [ ] 新增颜色支持（`printSuccess` / `printError` / `isTTY` 检测）
- [ ] 更新帮助文本

### Step 8: 跨平台兼容性加固

- [ ] 创建 `include/platform.hpp` 平台抽象层
- [ ] 散落的 `#ifdef _WIN32` 引用替换为 `platform.hpp`
- [ ] `AppConfig::defaults()` 按平台提供不同默认值
- [ ] 验证 Linux + Windows + macOS 编译

### Step 9: 编译测试

- [ ] cmake 配置
- [ ] 编译通过（无警告）
- [ ] 运行测试用例
- [ ] 功能验证：configure → 创建客户端 → 导出配置 → 校验内容

---

## 五、兼容性保证

| 模块     | 变化类型                                                                   | 兼容性                                                 |
| -------- | -------------------------------------------------------------------------- | ------------------------------------------------------ |
| C API 层 | 新增 `ovpn_mana_configure()` + `ovpn_mana_export_client_config()`      | ✅ 向后兼容，原有 API 不变                             |
| C++ 层   | `static` 方法 → 成员方法，`config.hpp` 全局常量 → `ovpn::defaults` | ⚠️ 仅 C++ 内部调用方受影响（目前只有 ovpn-mana.cpp） |
| CLI      | 新增 `--easy-rsa-dir`/`--ovpn-dir` 参数 + `client -export` 子命令    | ✅ 原有命令不受影响                                    |

**ABI 兼容性**：

- 新增导出函数不影响现有符号
- 数据结构布局无变化
- `ovpn_config_t` 新增结构体，旧调用方不引用即可
- 现有二次开发项目只需重新链接，无需重新编译

**运行时兼容性**：

- 不调用 `ovpn_mana_configure()` 时，行为与旧版完全一致（使用默认值）
- 调用 `ovpn_mana_configure()` 后，所有后续操作使用注入的路径

---

## 六、测试方案

### 6.1 单元测试

在 [test_client.cpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/test/test_client.cpp) 新增：

```cpp
// 测试命令模板替换（含运行时配置注入）
TEST(CommandTemplateTest, ReplaceBasic) {
    AppConfig cfg;
    cfg.easyRsaDir = "/test/easy-rsa";
    cfg.systemctlBin = "/bin/systemctl";

    std::string result = commands::replace(commands::EASYRSA_GEN_REQ, cfg, {
        {"NAME", "test-server"}
    });
    EXPECT_EQ(result, "cd /test/easy-rsa && ./easyrsa --batch gen-req test-server nopass");
}

// 测试配置注入
TEST(ConfigurationTest, Configure) {
    OpenVPNManager mgr;
    AppConfig cfg;
    cfg.ovpnDir = "/custom/openvpn";
    mgr.configure(cfg);
    EXPECT_EQ(mgr.config().ovpnDir, "/custom/openvpn");
    EXPECT_EQ(mgr.config().ovpnServerConfDir(), "/custom/openvpn/server");
}

// 测试默认配置
TEST(ConfigurationTest, DefaultConfig) {
    OpenVPNManager mgr;
    EXPECT_EQ(mgr.config().ovpnDir, "/etc/openvpn");
}

// 测试导出接口
TEST(ClientExportTest, ExportConfig) {
    // 需要在已创建客户端的环境运行
    // 验证两阶段调用能正确获取内容
}
```

### 6.2 功能测试流程

```bash
# 1. 创建服务（使用默认配置）
sudo ./openvpnmgr service -c test-srv,1194,10.8.0.0

# 2. 创建客户端
sudo ./openvpnmgr client -c test-srv,client1,1.2.3.4

# 3. 导出配置到文件
sudo ./openvpnmgr client -export test-srv,client1 > client1.ovpn

# 4. 验证文件存在且内容非空
ls -l client1.ovpn
grep -q "client" client1.ovpn

# 5. 使用自定义路径运行（运行时注入）
sudo ./openvpnmgr --easy-rsa-dir /opt/easy-rsa --ovpn-dir /etc/openvpn service -l
```

---

## 七、性能与稳定性风险

| 风险点                     | 影响                             | 应对措施                         |
| -------------------------- | -------------------------------- | -------------------------------- |
| 参数替换新增一次字符串拷贝 | 可忽略，命令执行本身是进程级开销 | -                                |
| 模板字符串存储在只读数据段 | 内存增加约 2KB，可忽略           | -                                |
| 大配置文件导出到用户缓冲区 | 配置文件通常 < 10KB，没问题      | 两阶段调用允许调用方分配正确大小 |
| 命令注入风险               | 原有代码也存在，重构不改变风险   | 建议：上层调用方做参数白名单校验 |

---

## 八、总结

| 需求                       | 完成方式                                                                               |
| -------------------------- | -------------------------------------------------------------------------------------- |
| 减少命令行字符串拼接       | 集中式 `constexpr` 模板 + `commands::replace()` 参数替换                           |
| 新增下载客户端配置导出函数 | C API `ovpn_mana_export_client_config` 支持两阶段获取                                |
| 编译期配置改为运行时注入   | 新增 `ovpn_config_t` + `ovpn_mana_configure()`，`AppConfig::defaults()` 保证兼容 |

重构后收益：

1. **可读性提升**：命令模板集中，语义清晰，无需在代码中散落字符串拼接
2. **可维护性提升**：修改命令不用到处搜索替换，一处修改全局生效
3. **部署灵活性**：同一 `.so` 可在不同环境运行，通过 `ovpn_mana_configure()` 注入路径
4. **多实例支持**：不同 handle 可注入不同路径，支持同时管理多个 easy-rsa 实例
5. **功能增强**：用户侧可直接获取 `.ovpn` 文件流用于下载
6. **兼容性**：完全向后兼容，不调用 `configure` 时行为与旧版一致

### 关键设计决策

| 决策点                 | 选择                                            | 理由                                            |
| ---------------------- | ----------------------------------------------- | ----------------------------------------------- |
| 模板引擎               | `constexpr` + 手动 `replace`                | 零依赖，编译期可见，适合≤50个模板              |
| 配置注入时机           | `ovpn_mana_create()` 之后，业务操作之前       | 与现有生命周期一致，无需新增 init 阶段          |
| `static` → 成员方法 | 全部改为成员方法                                | 每个 handle 持有独立配置，避免全局状态          |
| 默认值策略             | `AppConfig::defaults()` 硬编码                | 确保不调用 `configure` 时可用，与旧版行为一致 |
| 两阶段导出             | 先传 `nullptr` 获取大小，再传 buffer 获取内容 | 与 `get_online_clients` 模式一致，调用方熟悉  |

---

## 九、缺口分析：客户端固定 IP（CCD 支持）

### 9.1 现状

| 检查项                                              | 状态             | 说明                                                                                                     |
| --------------------------------------------------- | ---------------- | -------------------------------------------------------------------------------------------------------- |
| `createService` 创建 `ccd/` 目录                | ✅ 已实现        | [OpenVPNManager.cpp:L108-L118](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/OpenVPNManager.cpp#L108) |
| `createService` 写入 `client-config-dir` 指令   | ✅ 已实现        | [OpenVPNManager.cpp:L175](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/OpenVPNManager.cpp#L175)      |
| `createClient` 写入 CCD 文件（`ifconfig-push`） | ❌**缺失** | CCD 目录为空，无固定 IP 分配                                                                             |
| `ovpn_mana_create_client` 接受 client IP 参数     | ❌**缺失** | 当前签名 `(handle, service_name, name, wanip)` 无 client IP                                            |
| 重构规划中提及 CCD 固定 IP                          | ❌**缺失** | 模板中仅有 `client-config-dir` 指令，无 CCD 文件写入逻辑                                               |

**结论：CCD 基础设施已搭建，但缺少"写入 CCD 文件"这一关键步骤，无法为客户端分配固定 IP。**

### 9.2 OpenVPN CCD 固定 IP 原理

OpenVPN 的 `client-config-dir` 机制：

1. 服务端配置包含 `client-config-dir /path/to/ccd`
2. 在 `ccd/` 目录下，为每个需要固定 IP 的客户端创建文件，文件名为客户端 **Common Name**（即证书 CN）
3. 文件内容为 `ifconfig-push <client-ip> <server-ip>`

```bash
# 示例：ccd/client1 文件内容
ifconfig-push 10.8.0.10 10.8.0.1
```

### 9.3 建议方案

#### 9.3.1 接口变更

**C API 签名扩展**：

```cpp
// 现有签名（缺少 client_ip 参数）
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client(
    ovpn_mana_handle_t handle,
    const char *service_name,
    const char *name,
    const char* wanip);

// 建议新签名（可选 client_ip 参数，传 nullptr 表示不固定 IP）
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_client(
    ovpn_mana_handle_t handle,
    const char *service_name,
    const char *client_name,
    const char *wanip,
    const char *client_ip);   // 新增：固定 VPN IP，如 "10.8.0.10"，传 nullptr 表示自动分配
```

**`OpenVPNManager.hpp` 对应变更**：

```cpp
bool createClient(const std::string &name,
                  const std::string &serviceName,
                  const std::string &wanip,
                  const std::string &clientIp = "");  // 空字符串 = 自动分配
```

#### 9.3.2 核心实现（`createClient` 中追加）

```cpp
// 在 createClient 末尾，写入 .ovpn 文件之后，增加 CCD 文件写入：
if (!clientIp.empty()) {
    fs::path ccdFile = m_config.ovpnServerConfDir() + "/" + serviceName + "/ccd/" + name;
    std::ofstream ccdOut(ccdFile);
    if (!ccdOut.is_open()) {
        std::cerr << "Failed to create CCD file: " << ccdFile << std::endl;
        return false;
    }

    // 计算 server IP：取 clientIp 所在子网的 .1
    // 例如 clientIp="10.8.0.10" → serverIp="10.8.0.1"
    std::string serverIp = clientIp;
    size_t lastDot = serverIp.rfind('.');
    if (lastDot != std::string::npos) {
        serverIp = serverIp.substr(0, lastDot) + ".1";
    }

    ccdOut << "ifconfig-push " << clientIp << " " << serverIp << "\n";
    ccdOut.close();
    std::cout << "CCD file created for fixed IP: " << clientIp << std::endl;
}
```

#### 9.3.3 配套变更清单

| 文件                   | 变更                                                                                |
| ---------------------- | ----------------------------------------------------------------------------------- |
| `ovpn-mana.hpp`      | `ovpn_mana_create_client` 签名新增 `client_ip` 参数                             |
| `ovpn-mana.cpp`      | 传递 `client_ip` 到 `OpenVPNManager::createClient`                              |
| `OpenVPNManager.hpp` | `createClient` 签名新增 `clientIp` 参数（默认 `""`）                          |
| `OpenVPNManager.cpp` | 追加 CCD 文件写入逻辑                                                               |
| `main.cpp`           | `client -c` 解析增加可选的第4个字段 `<service_name>,<name>,<wanip>,<client_ip>` |
| `sdk.types.hpp`      | 无需变更（`ovpn_client_t` 已有 `private_ipv4` 字段）                            |

#### 9.3.4 兼容性策略

| 调用方                                     | 行为                           |
| ------------------------------------------ | ------------------------------ |
| 旧代码不传 `client_ip`（传 `nullptr`） | 行为不变，OpenVPN 自动分配 IP  |
| 新代码传入 `client_ip`                   | 写入 CCD 文件，固定 IP 分配    |
| `revoke_client` 删除客户端               | 需同步删除 `ccd/<name>` 文件 |

**关键**: `client_ip` 设为可选的最后一个参数，保证 ABI 兼容——旧调用方只需传 `nullptr`。

#### 9.3.5 IP 重复性校验方案

固定 IP 场景下，必须保证同一服务下不会有两个客户端分配相同的 VPN IP。重复性校验有四种候选方案：

| 方案                            | 原理                                                              | 复杂度 | 准确性          | 一致性风险                                     | 结论             |
| ------------------------------- | ----------------------------------------------------------------- | ------ | --------------- | ---------------------------------------------- | ---------------- |
| **A. 扫描 CCD 目录**      | 遍历 `ccd/` 下所有文件，解析 `ifconfig-push` 行，提取 IP 集合 | 低     | ✅ 精确         | 无（CCD 是唯一真实源）                         | ⭐**推荐** |
| **B. 扫描 ipp.txt**       | 解析 `ifconfig-pool-persist` 文件，提取已分配 IP                | 中     | ⚠️ 不完整     | 仅含已连接过的客户端，不含从未连接的新增客户端 | ❌               |
| **C. 解析 status.log**    | 解析 `status.log` 的 ROUTING TABLE 段                           | 中     | ⚠️ 仅在线     | 仅含当前在线客户端，离线客户端不出现           | ❌               |
| **D. 本地持久化（JSON）** | 在 `ovpnServerConfDir/<name>/ip_alloc.json` 维护 IP 分配记录    | 中     | ⚠️ 可能不一致 | 与 CCD 文件可能不同步（手动修改、异常中断）    | ❌ 不推荐        |

**最终选型：方案 A — 扫描 CCD 目录**

理由：

1. **CCD 是唯一真实源**：OpenVPN 只认 CCD 目录下的文件，扫描它天然保证一致性，不存在"本地持久化记录与 CCD 文件不同步"的问题
2. **实现简单**：仅需遍历目录 + 解析一行文本，无需引入额外状态文件
3. **性能可接受**：典型场景下 CCD 文件数量 ≤ 数千，遍历开销在毫秒级，且 `createClient` 本身就是秒级操作（easy-rsa 证书签发），遍历开销可忽略

**实现流程**：

```
ovpn_mana_create_client(handle, service_name, client_name, wanip, client_ip)
  │
  ├─ 1. client_ip == nullptr ?
  │     └─ 是 → 跳过校验，走自动分配流程
  │
  ├─ 2. 扫描 CCD 目录，收集已占用 IP 集合
  │     for each file in ccd/:
  │        读取首行，提取 ifconfig-push X.X.X.X 中的 IP
  │        加入 occupied_ips 集合
  │
  ├─ 3. client_ip ∈ occupied_ips ?
  │     └─ 是 → 返回 OVPN_ERR_IP_CONFLICT（新增错误码 -8）
  │
  └─ 4. IP 不冲突 → 继续正常创建流程（签发证书 → 写入 .ovpn → 写入 CCD 文件）
```

**CCD 文件扫描实现**：

```cpp
// 新增私有方法：收集指定服务下所有已通过 CCD 固定的 IP
std::set<std::string> OpenVPNManager::getFixedIps(const std::string& serviceName) {
    std::set<std::string> ips;
    fs::path ccdDir = m_config.ovpnServerConfDir() + "/" + serviceName + "/ccd";

    if (!fs::exists(ccdDir) || !fs::is_directory(ccdDir))
        return ips;

    for (const auto& entry : fs::directory_iterator(ccdDir)) {
        if (!entry.is_regular_file()) continue;

        std::ifstream f(entry.path());
        std::string line;
        if (!std::getline(f, line)) continue;

        // 解析 "ifconfig-push <client-ip> <server-ip>"
        if (line.rfind("ifconfig-push ", 0) != 0) continue;

        std::istringstream iss(line);
        std::string keyword, clientIp, serverIp;
        iss >> keyword >> clientIp >> serverIp;

        if (!clientIp.empty())
            ips.insert(clientIp);
    }
    return ips;
}
```

**在 `createClient` 中的调用位置**：

```cpp
bool OpenVPNManager::createClient(const std::string &name,
                                   const std::string &serviceName,
                                   const std::string &wanip,
                                   const std::string &clientIp) {
    // ====== 新增：IP 重复性校验 ======
    if (!clientIp.empty()) {
        auto occupiedIps = getFixedIps(serviceName);
        if (occupiedIps.count(clientIp)) {
            std::cerr << "IP conflict: " << clientIp << " is already assigned" << std::endl;
            return false;  // 或返回专门错误码，由 C API 层转换为 OVPN_ERR_IP_CONFLICT
        }
    }
    // ====== 原有逻辑继续 ======
    // ...
}
```

**错误码扩展**（[sdk.types.hpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/include/sdk.types.hpp)）：

```cpp
#define OVPN_ERR_IP_CONFLICT  -8   // 固定 IP 地址冲突
```

**边界情况**：

| 场景                             | 处理方式                                                                     |
| -------------------------------- | ---------------------------------------------------------------------------- |
| `clientIp` 为空字符串          | 跳过校验，OpenVPN 自动分配                                                   |
| CCD 目录不存在                   | 返回空集合（无冲突可能）                                                     |
| CCD 文件首行非 `ifconfig-push` | 跳过该文件（不参与校验）                                                     |
| 并发创建同一 IP                  | ⚠️**存在竞态**，两次调用可能同时通过校验。建议上层调用方串行化或加锁 |
| 客户端被吊销后重建               | 吊销时删除 CCD 文件，重建时不会冲突                                          |

**性能评估**（以 1000 个 CCD 文件为例）：

| 操作                        | 耗时估算                                      |
| --------------------------- | --------------------------------------------- |
| `directory_iterator` 遍历 | ~1ms                                          |
| 逐文件读取首行              | ~5ms（1000 × 5μs）                          |
| 总计                        | ~6ms（相对于 easy-rsa 证书签发 3-10s 可忽略） |

### 9.4 重构规划中待补充

在 `CommandTemplates.hpp` 的模板中增加：

```cpp
namespace ovpn::templates {

constexpr std::string_view CCD_FILE_CONTENT = R"(
ifconfig-push {CLIENT_IP} {SERVER_IP}
)";

} // namespace ovpn::templates
```

在改造步骤中增加 Step：

- [ ] **Step N: 新增 CCD 固定 IP 支持**
  - [ ] `sdk.types.hpp` 新增 `OVPN_ERR_IP_CONFLICT` 错误码（-8）
  - [ ] `OpenVPNManager.hpp` 新增 `getFixedIps()` 私有方法声明
  - [ ] `OpenVPNManager.cpp` 实现 `getFixedIps()`（扫描 CCD 目录）
  - [ ] `ovpn_mana_create_client` 签名新增可选 `client_ip` 参数
  - [ ] `OpenVPNManager::createClient` 追加 IP 重复性校验 + CCD 文件写入逻辑
  - [ ] `revokeClient` 追加删除 `ccd/<name>` 的逻辑
  - [ ] CLI `client -c` 支持可选的 IP 参数
  - [ ] 更新测试用例（验证 CCD 文件生成 + IP 冲突检测 + 吊销时清理）

---

## 十、兼容性回归审查

### 10.1 审查原则

重构遵循"**对外接口不变，内部实现可换**"原则。以下逐项审查每个变更对旧版本调用方的影响。

### 10.2 C API 层兼容性逐项审查

| 函数                                   | 旧签名                            | 新签名                                    | 兼容性                 | 说明                                       |
| -------------------------------------- | --------------------------------- | ----------------------------------------- | ---------------------- | ------------------------------------------ |
| `ovpn_mana_create`                   | `(void)`                        | `(void)`                                | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_destroy`                  | `(handle)`                      | `(handle)`                              | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_configure`                | 不存在                            | `(handle, config*)`                     | ✅ 新增函数            | 旧调用方不调用即可，行为与旧版一致         |
| `ovpn_mana_create_service`           | `(handle, name, subnet, port)`  | `(handle, name, subnet, port)`          | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_delete_service`           | `(handle, name)`                | `(handle, name)`                        | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_start_service`            | `(handle, name)`                | `(handle, name)`                        | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_stop_service`             | `(handle, name)`                | `(handle, name)`                        | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_restart_service`          | `(handle, name)`                | `(handle, name)`                        | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_list_services`            | `(handle, services, count)`     | `(handle, services, count)`             | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_create_client`            | `(handle, svc, name, wanip)`    | `(handle, svc, name, wanip, client_ip)` | ⚠️**签名变更** | 末尾新增可选参数，旧调用方需传 `nullptr` |
| `ovpn_mana_revoke_client`            | `(handle, svc, name)`           | `(handle, svc, name)`                   | ✅ 完全兼容            | 签名不变，内部增加删除 CCD 文件            |
| `ovpn_mana_get_online_clients`       | `(handle, svc, clients, count)` | `(handle, svc, clients, count)`         | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_get_online_clients_count` | `(handle, svc, count)`          | `(handle, svc, count)`                  | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_get_total_clients_count`  | `(handle, svc, count)`          | `(handle, svc, count)`                  | ✅ 完全兼容            | 签名不变                                   |
| `ovpn_mana_export_client_config`     | 不存在                            | `(handle, svc, name, buf, size)`        | ✅ 新增函数            | 新增导出功能                               |

**唯一破坏性变更**：`ovpn_mana_create_client` 末尾新增 `client_ip` 参数。

**缓解措施**：

| 调用方语言    | 适配方式                                             |
| ------------- | ---------------------------------------------------- |
| C             | 旧代码末尾追加 `NULL` 参数                         |
| C++           | 利用默认参数 `nullptr` 自动适配                    |
| Python/ctypes | 旧代码末尾追加 `None` 或 `ctypes.c_char_p(None)` |
| Node.js/ffi   | 旧代码末尾追加 `null`                              |

### 10.3 C++ 内部层兼容性审查

| 变更                                          | 影响范围                       | 兼容性            |
| --------------------------------------------- | ------------------------------ | ----------------- |
| `OpenVPNManager` 方法 static → 成员        | 仅 `ovpn-mana.cpp` 内部调用  | ✅ 对外无影响     |
| `config.hpp` 全局常量 → `ovpn::defaults` | 仅 `OpenVPNManager.cpp` 引用 | ✅ 对外无影响     |
| 命令字符串拼接 → 模板替换                    | 仅 `OpenVPNManager.cpp` 内部 | ✅ 对外无影响     |
| `AppConfig` 新增结构体                      | 新增类型                       | ✅ 不破坏现有类型 |

### 10.4 CLI 层兼容性审查

| 变更                                         | 旧命令                    | 兼容性                                    |
| -------------------------------------------- | ------------------------- | ----------------------------------------- |
| `--easy-rsa-dir` / `--ovpn-dir` 新增参数 | 不传时使用默认值          | ✅ 旧脚本不受影响                         |
| `client -export` 新增子命令                | 不存在的子命令            | ✅ 旧脚本不受影响                         |
| `client -c` 支持可选的 IP 参数             | `service,name,wanip,ip` | ✅ 旧格式 `service,name,wanip` 仍然有效 |
| 输出格式优化                                 | 排版变化                  | ✅ 不影响功能，仅视觉优化                 |

### 10.5 ABI 兼容性

| 检查项         | 状态                                        |
| -------------- | ------------------------------------------- |
| 导出符号新增   | ✅ 不影响现有符号                           |
| 结构体大小变化 | ✅ 仅新增 `ovpn_config_t`，现有结构体不变 |
| 虚函数表变化   | ✅`OpenVPNManager` 无虚函数               |
| SONAME 变更    | ✅ 保持 `libovpn-mana.so.1`               |

### 10.6 兼容性验证 checklist

- [ ] 旧版 C 调用方（仅调用 `create_service` 等基础 API）链接新版 `.so` 后功能正常
- [ ] 旧版 CLI 脚本（不传 `--easy-rsa-dir`）执行结果与旧版一致
- [ ] 旧版 `client -c service,name,wanip`（不传 IP）行为不变
- [ ] 新版 `client -c service,name,wanip,10.8.0.10` 正确写入 CCD
- [ ] 新版 `ovpn_mana_configure` 不调用时，默认路径与旧版 `config.hpp` 完全一致

---

## 十一、main.cpp 同步修改清单

> **实施原则**：`main.cpp` 的修改在重构中极易被遗漏。以下清单确保所有模块变更都在 CLI 层有对应的入口。

### 11.1 运行时配置注入 → main.cpp 适配

```cpp
// 新增 - 全局参数解析
#include "sdk.types.hpp"

// 新增 - 解析 --easy-rsa-dir 和 --ovpn-dir
ovpn_config_t cfg = {};
bool hasCustomConfig = false;

for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--easy-rsa-dir") == 0 && i + 1 < argc) {
        strncpy(cfg.easy_rsa_dir, argv[++i], sizeof(cfg.easy_rsa_dir) - 1);
        hasCustomConfig = true;
    } else if (strcmp(argv[i], "--ovpn-dir") == 0 && i + 1 < argc) {
        strncpy(cfg.ovpn_dir, argv[++i], sizeof(cfg.ovpn_dir) - 1);
        hasCustomConfig = true;
    }
    // ... 其余参数存到 args 数组
}

// 在 ovpn_mana_create() 之后立即调用
if (hasCustomConfig) {
    ovpn_mana_configure(handle, &cfg);
}
```

### 11.2 客户端配置导出 → main.cpp 适配

```cpp
// 新增 - client -export 子命令
else if (strcmp(argv[1], "client") == 0 && strcmp(argv[2], "-export") == 0) {
    // 解析 <service_name>,<client_name>
    std::string params = argv[3];
    size_t pos = params.find(',');
    std::string service_name = params.substr(0, pos);
    std::string client_name = params.substr(pos + 1);

    // 两阶段获取
    size_t buf_size = 0;
    ovpn_mana_export_client_config(handle, service_name.c_str(),
        client_name.c_str(), nullptr, &buf_size);
  
    std::vector<char> buf(buf_size);
    ovpn_mana_export_client_config(handle, service_name.c_str(),
        client_name.c_str(), buf.data(), &buf_size);
  
    std::cout.write(buf.data(), buf_size);
}
```

### 11.3 CCD 固定 IP → main.cpp 适配

```cpp
// 修改 - client -c 解析增加可选 IP 字段
// 旧格式: <service_name>,<name>,<wanip>
// 新格式: <service_name>,<name>,<wanip>,<client_ip>
std::vector<std::string> parts = split(params, ',');
if (parts.size() < 3) {
    std::cerr << "Invalid format. Use <service_name>,<name>,<wanip>[,<client_ip>]" << std::endl;
    return 1;
}
const char* client_ip = (parts.size() >= 4) ? parts[3].c_str() : nullptr;

ovpn_mana_create_client(handle, parts[0].c_str(), parts[1].c_str(),
    parts[2].c_str(), client_ip);
```

### 11.4 输出格式优化 → main.cpp 适配

参见 [第十三章](#十三maincpp-输出格式优化)。

### 11.5 main.cpp 完整修改点汇总

| 行范围                      | 修改类型 | 说明                                                |
| --------------------------- | -------- | --------------------------------------------------- |
| 新增参数解析                | 新增     | `--easy-rsa-dir` / `--ovpn-dir` + `--version` |
| `ovpn_mana_create()` 之后 | 新增     | `ovpn_mana_configure()` 调用                      |
| Banner 输出                 | 修改     | 替换为新的输出版面（第十三章）                      |
| `client -l` 输出          | 修改     | 表格化输出（第十三章）                              |
| `client -c` 解析          | 修改     | 支持可选 IP 字段                                    |
| `client -export`          | 新增     | 导出子命令                                          |
| 帮助文本                    | 修改     | 更新所有命令说明                                    |

---

## 十二、版本管理方案

### 12.1 方案选型

采用 **4 段版本号 + CMake 自动计算修订号** 方案，替代当前硬编码 `PROJECT_MAIN_VERSION 1.0.0.1`。

**版本号格式**：`MAJOR.MINOR.PATCH.DAYS`

| 段位  | 含义     | 变更规则                                                | 示例                   |
| ----- | -------- | ------------------------------------------------------- | ---------------------- |
| MAJOR | 主版本   | API/ABI 不兼容的大变更                                  | 1                      |
| MINOR | 次版本   | 新增功能，向后兼容                                      | 2                      |
| PATCH | 补丁版本 | 错误修复，向后兼容                                      | 3                      |
| DAYS  | 修订号   | **UTC 1970-01-01 以来的累计天数**，构建时自动计算 | 20626（即 2026-06-22） |

### 12.2 为什么用累计天数而非构建批次号

| 对比维度   | 旧方案 `PROJECT_MAKE_BATCH 250424` | 新方案 `DAYS = days_since_epoch` |
| ---------- | ------------------------------------ | ---------------------------------- |
| 人工维护   | 每次构建需手动修改                   | 完全自动，无需人工                 |
| 可追溯性   | 批次号无时间含义                     | 可直接反推构建日期                 |
| 单调性     | 人工可能忘记更新                     | 严格单调递增                       |
| CI/CD 友好 | 需要脚本修改 CMakeLists.txt          | 零配置                             |

### 12.3 CMakeLists.txt 实现

```cmake
cmake_minimum_required(VERSION 3.10)

# === 版本号定义 ===
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 2)
set(PROJECT_VERSION_PATCH 0)

# 自动计算 UTC 1970-01-01 以来的累计天数
string(TIMESTAMP PROJECT_VERSION_DAYS "%s" UTC)
math(EXPR PROJECT_VERSION_DAYS "${PROJECT_VERSION_DAYS} / 86400")

# 完整版本号
set(PROJECT_VERSION_FULL "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_DAYS}")

set(PROJECT_COMPANY_NAME "xuwh, xudev")
set(PROJECT_NAME openvpn-mana)

message(STATUS "Project Version: ${PROJECT_VERSION_FULL}")

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION_FULL} LANGUAGES C CXX)

# CMake project() 的 VERSION 只接受 3 段，DAYS 段通过独立编译宏注入
add_definitions(-DPROJECT_VERSION_DAYS=${PROJECT_VERSION_DAYS})
add_definitions(-DPROJECT_VERSION_FULL="${PROJECT_VERSION_FULL}")

# 生成 version.hpp
configure_file(
    "${PROJECT_SOURCE_DIR}/include/version.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/version.hpp"
    @ONLY
)

# SO 版本仍使用 MAJOR.MINOR.PATCH
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
    SOVERSION 1
)
```

### 12.4 version.hpp.in 模板

```cpp
// version.hpp.in -- 由 CMake configure_file() 生成，请勿手动编辑
#pragma once

#define OVPN_VERSION_MAJOR    @PROJECT_VERSION_MAJOR@
#define OVPN_VERSION_MINOR    @PROJECT_VERSION_MINOR@
#define OVPN_VERSION_PATCH    @PROJECT_VERSION_PATCH@
#define OVPN_VERSION_DAYS     @PROJECT_VERSION_DAYS@
#define OVPN_VERSION_FULL     "@PROJECT_VERSION_FULL@"
#define OVPN_COMPANY_NAME     "@PROJECT_COMPANY_NAME@"

namespace ovpn {

constexpr int kVersionMajor = @PROJECT_VERSION_MAJOR@;
constexpr int kVersionMinor = @PROJECT_VERSION_MINOR@;
constexpr int kVersionPatch = @PROJECT_VERSION_PATCH@;
constexpr int kVersionDays  = @PROJECT_VERSION_DAYS@;

inline const char* versionString() {
    return "@PROJECT_VERSION_FULL@";
}

// 构建日期反推：DAYS 转 YYYY-MM-DD
// 调用方可通过版本号直接反推构建日期，无需额外元数据

} // namespace ovpn
```

### 12.5 Windows 版本资源（version.rc.in）

```rc
// version.rc.in -- Windows 版本资源模板
#include <windows.h>

VS_VERSION_INFO VERSIONINFO
FILEVERSION    @PROJECT_VERSION_MAJOR@,@PROJECT_VERSION_MINOR@,@PROJECT_VERSION_PATCH@,@PROJECT_VERSION_DAYS@
PRODUCTVERSION @PROJECT_VERSION_MAJOR@,@PROJECT_VERSION_MINOR@,@PROJECT_VERSION_PATCH@,@PROJECT_VERSION_DAYS@
{
    BLOCK "StringFileInfo"
    {
        BLOCK "040904E4"
        {
            VALUE "CompanyName",      "@PROJECT_COMPANY_NAME@"
            VALUE "FileVersion",      "@PROJECT_VERSION_FULL@"
            VALUE "ProductVersion",   "@PROJECT_VERSION_FULL@"
            VALUE "FileDescription",  "OpenVPN Manager SDK"
            VALUE "ProductName",      "libovpn-mana"
        }
    }
}
```

### 12.6 CLI --version 输出

```cpp
// main.cpp 新增
else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
    std::cout << "openvpn-mana version " << OVPN_VERSION_FULL << std::endl;
    std::cout << "Build: " << OVPN_VERSION_DAYS << " days since epoch" << std::endl;
    return 0;
}
```

输出示例：

```
openvpn-mana version 1.2.0.20626
Build: 20626 days since epoch
```

---

## 十三、main.cpp 输出格式优化

### 13.1 现状问题

当前输出使用 `=====` 分隔线和 `\t\t` 缩进，风格粗糙：

```
=====================================================================================
		OpenVPN Command Line Manager
=====================================================================================
```

在线客户端列表为单行长文本，87 个客户端时几乎不可读：

```
  Name: pi-2ccf6797a654, Private IP: 10.7.0.56, Public IP: 111.55.197.255:43244, ...
```

### 13.2 优化目标

- 专业的企业级 CLI 工具风格（参考 `docker ps`、`kubectl get pods`）
- 表格化对齐输出，固定列宽，关键信息一目了然
- 支持彩色输出（可配置开关）
- 大数据量时自动分页或截断

### 13.3 优化后 Banner

```
+==================================================================+
|  OpenVPN Manager  v1.2.0.20626                                   |
|  Copyright (c) 2026 xuwh, xudev                                  |
+==================================================================+
```

**实现**：

```cpp
// main.cpp 新增
void printBanner() {
    std::string version = OVPN_VERSION_FULL;
    std::cout << "+==================================================================+\n"
              << "|  OpenVPN Manager  v" << version;
    size_t padding = 67 - 22 - version.length();
    std::cout << std::string(padding, ' ') << "|\n"
              << "|  Copyright (c) 2026 xuwh, xudev                                    |\n"
              << "+==================================================================+\n";
}
```

### 13.4 优化后客户端列表

```
OpenVPN Clients for service 'admin':
+----------------------------------+--------------+----------------------+-------------------+--------------+--------------+
| Client Name                      | Private IP   | Public IP:Port       | Connected Since   | Rx           | Tx           |
+----------------------------------+--------------+----------------------+-------------------+--------------+--------------+
| af6867d2-d669-8c98-fcce-3a...    | 10.7.0.26    | 39.144.90.152:33332  | 2026-06-16 11:13  | 20.6 MiB     | 30.4 MiB     |
| DESKTOP-6P2IRVL                  | 10.7.0.59    | 39.144.90.178:10118  | 2026-06-16 11:13  | 7.7 MiB      | 19.0 MiB     |
| pi-2ccf6797a654                  | 10.7.0.56    | 111.55.197.255:43244 | 2026-06-16 11:13  | 2.6 MiB      | 13.0 MiB     |
+----------------------------------+--------------+----------------------+-------------------+--------------+--------------+
Total: 87 clients  |  Online: 39  |  Offline: 48
```

**实现**：

```cpp
// main.cpp 新增
#include <iomanip>
#include <cmath>

// 人类可读的字节数
std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    int unit = 0;
    double value = static_cast<double>(bytes);
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        unit++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value << " " << units[unit];
    return oss.str();
}

// 截断过长名称
std::string truncate(const std::string& s, size_t maxLen) {
    if (s.length() <= maxLen) return s;
    return s.substr(0, maxLen - 3) + "...";
}

// 表格列宽常量
constexpr int COL_NAME     = 32;
constexpr int COL_PRIV_IP  = 12;
constexpr int COL_PUB_IP   = 20;
constexpr int COL_SINCE    = 17;
constexpr int COL_RX       = 12;
constexpr int COL_TX       = 12;

// 打印水平分隔线
void printSeparator(char left, char mid, char right, char fill = '-') {
    std::cout << left
              << std::string(COL_NAME,    fill) << mid
              << std::string(COL_PRIV_IP, fill) << mid
              << std::string(COL_PUB_IP,  fill) << mid
              << std::string(COL_SINCE,   fill) << mid
              << std::string(COL_RX,      fill) << mid
              << std::string(COL_TX,      fill) << right << '\n';
}

void printTableHeader() {
    printSeparator('+', '+', '+', '-');
    std::cout << "|" << std::setw(COL_NAME)    << std::left << " Client Name"
              << "|" << std::setw(COL_PRIV_IP) << std::left << " Private IP"
              << "|" << std::setw(COL_PUB_IP)  << std::left << " Public IP:Port"
              << "|" << std::setw(COL_SINCE)   << std::left << " Connected Since"
              << "|" << std::setw(COL_RX)      << std::left << " Rx"
              << "|" << std::setw(COL_TX)      << std::left << " Tx"
              << "|\n";
    printSeparator('+', '+', '+', '-');
}

void printClientRow(const ovpn_client_t& c) {
    std::string pubAddr = std::string(c.public_ipv4) + ":" + std::to_string(c.public_port);
    std::string since = std::string(c.since).substr(0, 16);
    std::cout << "|" << std::setw(COL_NAME)    << std::left << truncate(c.name, COL_NAME)
              << "|" << std::setw(COL_PRIV_IP) << std::left << c.private_ipv4
              << "|" << std::setw(COL_PUB_IP)  << std::left << pubAddr
              << "|" << std::setw(COL_SINCE)   << std::left << since
              << "|" << std::setw(COL_RX)      << std::left << formatBytes(c.bytes_received)
              << "|" << std::setw(COL_TX)      << std::left << formatBytes(c.bytes_sent)
              << "|\n";
}

void printTableFooter() {
    printSeparator('+', '+', '+', '-');
}

// 在 client -l 处理中使用
std::cout << "OpenVPN Clients for service '" << service_name << "':\n";
printTableHeader();
for (int i = 0; i < client_count; i++) {
    printClientRow(clients[i]);
}
printTableFooter();
std::cout << "Total: " << total_count << " clients  |  "
          << "Online: " << client_count << "  |  "
          << "Offline: " << (total_count - client_count) << "\n";
```

### 13.5 颜色与交互

```cpp
// 颜色常量（可配置）
namespace color {
    constexpr const char* RESET   = "\033[0m";
    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* BOLD    = "\033[1m";
}

// 成功/失败消息
void printSuccess(const std::string& msg) {
    std::cout << color::GREEN << "[OK] " << msg << color::RESET << "\n";
}
void printError(const std::string& msg) {
    std::cerr << color::RED << "[ERR] " << msg << color::RESET << "\n";
}

// 检测输出是否为终端（非终端时禁用颜色，如管道到文件）
bool isTTY() {
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdout));
#endif
}
```

### 13.6 优化前后对比

| 维度       | 优化前                                       | 优化后                                    |
| ---------- | -------------------------------------------- | ----------------------------------------- |
| Banner     | `=====` 分隔线 + `\t\t` 缩进             | ASCII 框线 + 版本号 + 版权                |
| 客户端列表 | 单行长文本 `Name:..., Private IP:..., ...` | 表格对齐，6 列固定列宽                    |
| 字节数     | 原始数字 `21635661`                        | 人类可读 `20.6 MiB`                     |
| 长名称     | 完整输出，破坏对齐                           | 自动截断 `pi-2ccf6797...`               |
| 颜色       | 无                                           | 成功绿/错误红/标题青（非 TTY 时自动禁用） |
| CLI 体验   | 业余工具风格                                 | 专业运维工具风格                          |

---

## 十四、跨平台兼容性保障

### 14.1 现状分析

当前代码已做初步跨平台处理：

| 文件                           | 处理方式                                           | 覆盖范围           |
| ------------------------------ | -------------------------------------------------- | ------------------ |
| `main.cpp:L27`               | `#if defined(__WIN32__) ...` 条件编译            | Windows 权限检查   |
| `ovpn-mana.hpp:L15`          | `#if defined(__WIN32__) ...` 条件编译            | DLL 导出宏         |
| `OpenVPNManager.cpp:L12-L15` | `#include <unistd.h>` + `#define popen _popen` | Windows popen 兼容 |

**问题**：跨平台处理散落在各文件中，缺乏统一抽象层，新增平台时需逐个文件修改。

### 14.2 平台抽象层设计

新增 `include/platform.hpp`，集中管理所有平台差异：

```cpp
// platform.hpp -- 跨平台抽象层
#pragma once

// ============================================================
// 平台检测
// ============================================================
#if defined(_WIN32) || defined(_WIN64)
    #define OVPN_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define OVPN_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define OVPN_PLATFORM_MACOS 1
#else
    #error "Unsupported platform"
#endif

// ============================================================
// 头文件
// ============================================================
#if OVPN_PLATFORM_WINDOWS
    #include <windows.h>
    #include <io.h>
    #define popen  _popen
    #define pclose _pclose
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
#endif

// ============================================================
// 路径分隔符
// ============================================================
#if OVPN_PLATFORM_WINDOWS
    constexpr char PATH_SEP = '\\';
    constexpr char PATH_SEP_OTHER = '/';
#else
    constexpr char PATH_SEP = '/';
    constexpr char PATH_SEP_OTHER = '\\';
#endif

// ============================================================
// 权限检查
// ============================================================
namespace ovpn::platform {

inline bool isRoot() {
#if OVPN_PLATFORM_WINDOWS
    BOOL isElevated = FALSE;
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
            isElevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return isElevated != FALSE;
#else
    return geteuid() == 0;
#endif
}

// 获取可执行文件扩展名
inline const char* exeExtension() {
#if OVPN_PLATFORM_WINDOWS
    return ".exe";
#else
    return "";
#endif
}

// 规范化路径（统一使用当前平台分隔符）
inline std::string normalizePath(const std::string& path) {
    std::string result = path;
    for (auto& ch : result) {
        if (ch == PATH_SEP_OTHER) ch = PATH_SEP;
    }
    return result;
}

} // namespace ovpn::platform
```

### 14.3 各模块跨平台改造点

#### 14.3.1 OpenVPNManager.cpp

```cpp
// 改造前（散落的平台判断）
#include <unistd.h>
#ifdef _WIN32
#define popen _popen
#endif

// 改造后（统一引用）
#include "platform.hpp"

// execCommand 中 popen/pclose 已在 platform.hpp 中统一处理
std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
```

**注意**：`systemctl`、`sudo`、`easy-rsa` 等命令是 Linux 特有的。Windows 上需要替代方案：

| Linux 命令               | Windows 替代                                       | 说明                 |
| ------------------------ | -------------------------------------------------- | -------------------- |
| `systemctl start/stop` | `sc start/stop` 或 `net start/stop`            | 服务管理             |
| `sudo`                 | 以管理员权限运行即可                               | Windows 无 sudo 概念 |
| `easy-rsa`             | 需安装 Windows 版 easy-rsa 或使用 OpenSSL 直接操作 | 证书管理             |
| `openvpn --genkey`     | Windows 版 OpenVPN 支持相同参数                    | 密钥生成             |

> **建议**：在 `AppConfig` 中增加 `useSystemctl` 和 `useSudo` 布尔标志，Windows 上自动设为 `false`。

#### 14.3.2 AppConfig 跨平台默认值

```cpp
struct AppConfig {
    // ... 现有字段 ...

    // 平台相关默认值
    static AppConfig defaults() {
        AppConfig cfg;
#if OVPN_PLATFORM_WINDOWS
        cfg.easyRsaDir    = "C:\\Program Files\\OpenVPN\\easy-rsa";
        cfg.ovpnDir       = "C:\\Program Files\\OpenVPN\\config";
        cfg.openvpnBin    = "C:\\Program Files\\OpenVPN\\bin\\openvpn.exe";
        cfg.systemctlBin  = "";  // Windows 不使用 systemctl
        cfg.useSudo       = false;
        cfg.useSystemctl  = false;
#else
        cfg.easyRsaDir    = "/home/xuwh/easy-rsa";
        cfg.ovpnDir       = "/etc/openvpn";
        cfg.openvpnBin    = "/usr/sbin/openvpn";
        cfg.systemctlBin  = "/bin/systemctl";
        cfg.useSudo       = true;
        cfg.useSystemctl  = true;
#endif
        return cfg;
    }

    bool useSudo      = true;
    bool useSystemctl = true;
};
```

#### 14.3.3 文件系统操作

```cpp
// 全部使用 std::filesystem（C++17），天然跨平台
#include <filesystem>
namespace fs = std::filesystem;

// 目录创建 -- 跨平台
fs::create_directories(ccdDir);

// 路径拼接 -- 跨平台
fs::path configPath = fs::path(cfg.ovpnDir) / "server" / (name + ".conf");

// 文件遍历 -- 跨平台
for (const auto& entry : fs::directory_iterator(ccdDir)) { ... }
```

### 14.4 跨平台验证 checklist

| 平台             | 编译器     | 验证项                    |
| ---------------- | ---------- | ------------------------- |
| Ubuntu 20.04+    | GCC 9+     | 编译 + 全功能测试         |
| CentOS 7/8       | GCC 8+     | 编译 + systemctl 集成测试 |
| Windows 10/11    | MSVC 2019+ | 编译 + sc 命令测试        |
| Windows 10/11    | MinGW-w64  | 编译 + 功能验证           |
| macOS 12+ (可选) | Clang 14+  | 编译验证                  |

### 14.5 CMakeLists.txt 跨平台配置

```cmake
# 平台检测
if(WIN32)
    set(OVPN_PLATFORM "Windows")
    set(PLATFORM_SRCS src/platform_win.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE advapi32)
elseif(APPLE)
    set(OVPN_PLATFORM "macOS")
    set(PLATFORM_SRCS src/platform_unix.cpp)
else()
    set(OVPN_PLATFORM "Linux")
    set(PLATFORM_SRCS src/platform_unix.cpp)
endif()

message(STATUS "Target Platform: ${OVPN_PLATFORM}")

# 生成 platform_config.hpp
configure_file(
    "${PROJECT_SOURCE_DIR}/include/platform_config.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/platform_config.hpp"
    @ONLY
)
```

### 14.6 跨平台兼容性总结

| 模块     | 改造前                      | 改造后                                  |
| -------- | --------------------------- | --------------------------------------- |
| 平台检测 | 散落 `#ifdef`             | 统一 `platform.hpp`                   |
| 权限检查 | `geteuid()`               | `ovpn::platform::isRoot()`            |
| 路径处理 | 硬编码 `/`                | `std::filesystem` + `normalizePath` |
| 命令执行 | `popen` / `_popen` 分散 | `platform.hpp` 统一处理               |
| 服务管理 | 硬编码 `systemctl`        | 配置驱动 `AppConfig::useSystemctl`    |
| 默认配置 | 仅 Linux 路径               | 按平台提供不同默认值                    |

---

## 十五、参数校验方案

### 15.1 现状与风险

当前 `createService`、`createClient` 等接口对传入参数**没有任何校验**，直接将调用方传入的字符串拼接到 shell 命令中：

```cpp
// 现状：name 直接拼入命令，无任何校验
std::string cmd = "cd " + EASY_RSA_DIR + " && ./easyrsa --batch gen-req " + name + "-server nopass";
```

**风险矩阵**：

| 风险类型     | 攻击向量                                   | 危害等级 | 示例                                            |
| ------------ | ------------------------------------------ | -------- | ----------------------------------------------- |
| 命令注入     | `name` 包含 `; rm -rf /` 或 `$(...)` | 🔴 严重  | `name = "test; rm -rf /etc/openvpn"`          |
| 路径穿越     | `name` 包含 `../`                      | 🔴 严重  | `name = "../../etc/passwd"`                   |
| 脏数据落盘   | `name` 包含不可打印字符/控制字符         | 🟡 中等  | `name = "test\x00server"` 导致截断            |
| 文件系统冲突 | `name` 包含 `/` `\` 等路径分隔符     | 🟡 中等  | `name = "a/b"` 被误解析为子目录               |
| 配置解析错误 | `name` 包含 `"` `'` 等引号字符       | 🟡 中等  | `name = "test\"server"` 破坏配置语法          |
| IP 格式错误  | `client_ip` 传入非法 IP 字符串           | 🟡 中等  | `client_ip = "999.999.999.999"`               |
| 资源耗尽     | `name` 超长（如 10KB 字符串）            | 🟢 低    | `name = std::string(10000, 'A')` 导致路径过长 |

### 15.2 校验位置

校验应在 **SDK 内部入口处** 统一执行，而非依赖调用方自觉：

```
调用方（Python / Node.js / CLI）
  │
  │  ovpn_mana_create_service(handle, name, subnet, port)
  │
  ▼
┌─────────────────────────────────────────┐
│  ovpn_mana_create_service()             │
│    │                                     │
│    ├─ validateServiceName(name)    ◄── 校验点 1 │
│    ├─ validateSubnet(subnet)       ◄── 校验点 2 │
│    ├─ validatePort(port)           ◄── 校验点 3 │
│    │                                     │
│    └─ mgr->createService(...)           │
└─────────────────────────────────────────┘
```

### 15.3 校验规则定义

#### 15.3.1 服务名称 / 客户端名称校验

```cpp
// include/Validators.hpp
#pragma once
#include <string>
#include <string_view>
#include <regex>
#include <cstdint>

namespace ovpn::validators {

// 名称约束常量
constexpr size_t MAX_NAME_LENGTH = 64;       // 最大长度
constexpr size_t MIN_NAME_LENGTH = 1;         // 最小长度（非空）

// 允许的字符集：字母、数字、连字符、下划线、点号
// 对应 OpenVPN 配置文件命名惯例，同时避免 shell 特殊字符
constexpr std::string_view NAME_PATTERN = R"(^[a-zA-Z0-9][a-zA-Z0-9_.\-]*$)";

// 显式禁止的 shell 元字符（双重保险，即使正则已覆盖）
constexpr std::string_view FORBIDDEN_CHARS = ";&|`$(){}[]<>!\\\"' \t\n\r";

struct ValidationResult {
    bool valid = false;
    std::string reason;  // 失败原因，用于日志和错误返回
};

// 名称校验
ValidationResult validateName(const std::string& name,
                              const std::string& fieldName = "name");

// 服务名称校验（额外约束：不得以 "-server" 结尾，避免与 OpenVPN 内部命名冲突）
ValidationResult validateServiceName(const std::string& name);

// 客户端名称校验（额外约束：不得与已有客户端重名）
ValidationResult validateClientName(const std::string& name);

} // namespace ovpn::validators
```

#### 15.3.2 IPv4 地址校验

```cpp
// 在 Validators.hpp 中继续

// 校验 IPv4 地址格式：x.x.x.x，每段 0-255，不含前导零（除 "0" 本身）
ValidationResult validateIPv4(const std::string& ip,
                              const std::string& fieldName = "ip");

// 校验子网地址：x.x.x.x/nn，其中 nn 为 8-30 的 CIDR 前缀长度
ValidationResult validateSubnet(const std::string& subnet,
                                const std::string& fieldName = "subnet");

// 校验端口号：1024-65535（避免特权端口）
ValidationResult validatePort(int port,
                              const std::string& fieldName = "port");
```

#### 15.3.3 校验规则详细定义

| 参数类型               | 规则                                                                 | 正面示例                                    | 反面示例                                           |
| ---------------------- | -------------------------------------------------------------------- | ------------------------------------------- | -------------------------------------------------- |
| **name**         | 长度 1-64，匹配 `[a-zA-Z0-9][a-zA-Z0-9_.\-]*`，不含 shell 特殊字符 | `my-server`、`client_01`、`vpn.node3` | `test;rm`、`a/b`、`"inject"`、`$(whoami)`  |
| **service_name** | 同 name + 不得以 `-server` 结尾                                    | `my-vpn`、`prod-gw`                     | `my-server`（与 OpenVPN 内部后缀冲突）           |
| **client_name**  | 同 name + 不得为空                                                   | `laptop-alice`、`pi-node-7`             | `""`、`*`、`../etc`                          |
| **ipv4**         | 4 段十进制，每段 0-255，无前导零                                     | `10.8.0.5`、`192.168.1.1`               | `10.8.0.256`、`010.008.000.005`、`not-an-ip` |
| **subnet**       | 格式 `x.x.x.x/nn`，nn∈[8,30]，主机位为 0                          | `10.8.0.0/24`、`172.16.0.0/16`          | `10.8.0.0/33`、`10.8.0.5/24`（主机位非零）     |
| **port**         | 整数，范围 1024-65535                                                | `1194`、`8443`                          | `0`、`80`、`99999`、`-1`                   |

### 15.4 校验实现

#### 15.4.1 新增 `Validators.cpp`

```cpp
// src/Validators.cpp
#include "Validators.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace ovpn::validators {

static bool isForbiddenChar(char ch) {
    return FORBIDDEN_CHARS.find(ch) != std::string_view::npos;
}

static bool isPrintableAscii(const std::string& s) {
    for (char ch : s) {
        unsigned char uc = static_cast<unsigned char>(ch);
        if (uc < 0x20 || uc > 0x7E) return false;  // 仅允许可打印 ASCII
    }
    return true;
}

ValidationResult validateName(const std::string& name,
                              const std::string& fieldName)
{
    ValidationResult result;

    if (name.empty()) {
        result.reason = fieldName + " must not be empty";
        return result;
    }

    if (name.length() > MAX_NAME_LENGTH) {
        result.reason = fieldName + " exceeds maximum length of " +
                        std::to_string(MAX_NAME_LENGTH) + " characters";
        return result;
    }

    if (!isPrintableAscii(name)) {
        result.reason = fieldName + " contains non-printable characters";
        return result;
    }

    if (std::any_of(name.begin(), name.end(), isForbiddenChar)) {
        result.reason = fieldName + " contains forbidden shell characters";
        return result;
    }

    // 正则校验：字母数字开头，后续可含 _ . -
    static const std::regex nameRegex(R"(^[a-zA-Z0-9][a-zA-Z0-9_.\-]*$)");
    if (!std::regex_match(name, nameRegex)) {
        result.reason = fieldName + " must start with alphanumeric and contain only [a-zA-Z0-9_.-]";
        return result;
    }

    result.valid = true;
    return result;
}

ValidationResult validateServiceName(const std::string& name)
{
    auto result = validateName(name, "service_name");
    if (!result.valid) return result;

    // 服务名不得以 "-server" 结尾，避免与 OpenVPN 内部命名冲突
    constexpr std::string_view suffix = "-server";
    if (name.length() >= suffix.length() &&
        name.compare(name.length() - suffix.length(), suffix.length(), suffix) == 0) {
        result.valid = false;
        result.reason = "service_name must not end with '-server' (reserved suffix)";
        return result;
    }

    return result;
}

ValidationResult validateClientName(const std::string& name)
{
    return validateName(name, "client_name");
}

ValidationResult validateIPv4(const std::string& ip,
                              const std::string& fieldName)
{
    ValidationResult result;

    if (ip.empty()) {
        result.reason = fieldName + " must not be empty";
        return result;
    }

    // 手动解析 IPv4，避免 inet_pton 的平台差异
    std::vector<int> octets;
    std::istringstream iss(ip);
    std::string token;

    // 检查前导零（如 "01" 或 "010"）
    for (size_t i = 0; i < ip.size(); i++) {
        if (ip[i] == '0' && (i == 0 || ip[i-1] == '.') &&
            i + 1 < ip.size() && ip[i+1] != '.') {
            result.reason = fieldName + " contains leading zeros in octet";
            return result;
        }
    }

    while (std::getline(iss, token, '.')) {
        if (token.empty() || token.size() > 3) {
            result.reason = fieldName + " has invalid octet format";
            return result;
        }
        for (char ch : token) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                result.reason = fieldName + " contains non-digit characters";
                return result;
            }
        }
        int val = std::stoi(token);
        if (val < 0 || val > 255) {
            result.reason = fieldName + " octet out of range [0, 255]";
            return result;
        }
        octets.push_back(val);
    }

    if (octets.size() != 4) {
        result.reason = fieldName + " must have exactly 4 octets";
        return result;
    }

    result.valid = true;
    return result;
}

ValidationResult validateSubnet(const std::string& subnet,
                                const std::string& fieldName)
{
    ValidationResult result;

    // 格式：x.x.x.x/nn
    size_t slashPos = subnet.find('/');
    if (slashPos == std::string::npos) {
        result.reason = fieldName + " missing CIDR prefix (expected x.x.x.x/nn)";
        return result;
    }

    std::string ipPart = subnet.substr(0, slashPos);
    std::string prefixPart = subnet.substr(slashPos + 1);

    auto ipResult = validateIPv4(ipPart, fieldName + " IP part");
    if (!ipResult.valid) {
        return ipResult;
    }

    // 校验前缀长度
    if (prefixPart.empty()) {
        result.reason = fieldName + " has empty CIDR prefix";
        return result;
    }
    for (char ch : prefixPart) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            result.reason = fieldName + " CIDR prefix must be numeric";
            return result;
        }
    }
    int prefix = std::stoi(prefixPart);
    if (prefix < 8 || prefix > 30) {
        result.reason = fieldName + " CIDR prefix must be in range [8, 30]";
        return result;
    }

    // 校验主机位是否为 0（子网地址而非主机地址）
    // 例如 10.8.0.5/24 非法，10.8.0.0/24 合法
    uint32_t ipInt = 0;
    {
        std::istringstream iss(ipPart);
        std::string token;
        int shift = 24;
        while (std::getline(iss, token, '.')) {
            ipInt |= (static_cast<uint32_t>(std::stoi(token)) << shift);
            shift -= 8;
        }
    }
    uint32_t hostMask = (1u << (32 - prefix)) - 1;
    if (ipInt & hostMask) {
        result.reason = fieldName + " has non-zero host bits (expected subnet base address)";
        return result;
    }

    result.valid = true;
    return result;
}

ValidationResult validatePort(int port, const std::string& fieldName)
{
    ValidationResult result;

    if (port < 1024 || port > 65535) {
        result.reason = fieldName + " must be in range [1024, 65535], got " +
                        std::to_string(port);
        return result;
    }

    result.valid = true;
    return result;
}

} // namespace ovpn::validators
```

### 15.5 校验集成点

#### 15.5.1 C API 层集成（`ovpn-mana.cpp`）

```cpp
// 在 ovpn_mana_create_service 中增加校验
LIB_API ovpn_err_t LIB_API_CALL ovpn_mana_create_service(
    ovpn_mana_handle_t handle,
    const char *service_name,
    const char *subnet,
    int port)
{
    try {
        if (!handle || !service_name || !subnet) {
            return OVPN_ERR_INVALID_PARAM;
        }

        auto *mgr = reinterpret_cast<OpenVPNManager*>(handle);

        // ─── 参数校验 ───
        auto nameResult = validators::validateServiceName(service_name);
        if (!nameResult.valid) {
            std::cerr << "[VALIDATION] " << nameResult.reason << std::endl;
            return OVPN_ERR_INVALID_PARAM;
        }

        auto subnetResult = validators::validateSubnet(subnet);
        if (!subnetResult.valid) {
            std::cerr << "[VALIDATION] " << subnetResult.reason << std::endl;
            return OVPN_ERR_INVALID_PARAM;
        }

        auto portResult = validators::validatePort(port);
        if (!portResult.valid) {
            std::cerr << "[VALIDATION] " << portResult.reason << std::endl;
            return OVPN_ERR_INVALID_PARAM;
        }
        // ─── 校验结束 ───

        if (mgr->createService(service_name, subnet, port)) {
            return OVPN_ERR_SUCCESS;
        }
        return OVPN_ERR_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create service: " << e.what() << std::endl;
        return OVPN_ERR_FAILURE;
    }
}
```

#### 15.5.2 其他需校验的 API

| API                                | 校验参数                                                 | 校验规则                                                                      |
| ---------------------------------- | -------------------------------------------------------- | ----------------------------------------------------------------------------- |
| `ovpn_mana_create_service`       | `service_name`, `subnet`, `port`                   | `validateServiceName` + `validateSubnet` + `validatePort`               |
| `ovpn_mana_delete_service`       | `service_name`                                         | `validateServiceName`（防止路径穿越）                                       |
| `ovpn_mana_create_client`        | `service_name`, `client_name`, `client_ip`（可选） | `validateServiceName` + `validateClientName` + `validateIPv4`（若提供） |
| `ovpn_mana_delete_client`        | `service_name`, `client_name`                        | `validateServiceName` + `validateClientName`                              |
| `ovpn_mana_start_service`        | `service_name`                                         | `validateServiceName`                                                       |
| `ovpn_mana_stop_service`         | `service_name`                                         | `validateServiceName`                                                       |
| `ovpn_mana_export_client_config` | `service_name`, `client_name`                        | `validateServiceName` + `validateClientName`                              |
| `ovpn_mana_list_clients`         | `service_name`                                         | `validateServiceName`                                                       |

### 15.6 错误码扩展

为支持校验失败时返回精确错误码，在 `sdk.types.hpp` 中扩展：

```cpp
// 在 sdk.types.hpp 中新增（与现有 OVPN_ERR_SUCCESS 等并列）
#define OVPN_ERR_INVALID_PARAM      -1001   // 参数校验失败（名称/格式非法）
#define OVPN_ERR_NAME_TOO_LONG      -1002   // 名称过长
#define OVPN_ERR_IP_FORMAT          -1003   // IP 地址格式错误
#define OVPN_ERR_PORT_RANGE         -1004   // 端口号超出范围
#define OVPN_ERR_FORBIDDEN_CHAR     -1005   // 包含禁止字符
```

> **注意**：当前代码中已有 `OVPN_ERR_INVALID_PARAM`（见 [ovpn-mana.cpp](file:///d:/Projects/20250427-ovn-mana/sdk-gitee/src/ovpn-mana.cpp)），需确认其值并在此基础上扩展子错误码，或通过 `ovpn_mana_get_last_error()` 返回详细原因字符串。

### 15.7 校验时机与性能

| 校验规则                | 复杂度             | 性能影响 |
| ----------------------- | ------------------ | -------- |
| 长度检查                | O(1)               | 可忽略   |
| 字符集检查（遍历+正则） | O(n)，n ≤ 64      | ~1μs    |
| IPv4 解析               | O(1)，固定 15 字符 | ~1μs    |
| 子网解析                | O(1)               | ~2μs    |

**结论**：所有校验均在微秒级完成，不影响 API 响应时间。校验在 shell 命令拼接之前执行，不引入额外 IO。

### 15.8 校验函数的单元测试

```cpp
// tests/unit/test_validators.cpp
#include <gtest/gtest.h>
#include "Validators.hpp"

using namespace ovpn::validators;

// ========== validateName ==========
TEST(ValidateName, AcceptsValidNames) {
    EXPECT_TRUE(validateName("my-server").valid);
    EXPECT_TRUE(validateName("client_01").valid);
    EXPECT_TRUE(validateName("vpn.node3").valid);
    EXPECT_TRUE(validateName("a").valid);                     // 最短合法名称
    EXPECT_TRUE(validateName(std::string(64, 'a')).valid);    // 最长合法名称
}

TEST(ValidateName, RejectsEmpty) {
    EXPECT_FALSE(validateName("").valid);
}

TEST(ValidateName, RejectsTooLong) {
    EXPECT_FALSE(validateName(std::string(65, 'a')).valid);
}

TEST(ValidateName, RejectsShellMetacharacters) {
    EXPECT_FALSE(validateName("test;rm").valid);
    EXPECT_FALSE(validateName("test$(whoami)").valid);
    EXPECT_FALSE(validateName("test`id`").valid);
    EXPECT_FALSE(validateName("test|cat").valid);
    EXPECT_FALSE(validateName("test&").valid);
}

TEST(ValidateName, RejectsPathTraversal) {
    EXPECT_FALSE(validateName("../etc").valid);
    EXPECT_FALSE(validateName("a/b").valid);
    EXPECT_FALSE(validateName("a\\b").valid);
}

TEST(ValidateName, RejectsQuotes) {
    EXPECT_FALSE(validateName("test\"name").valid);
    EXPECT_FALSE(validateName("test'name").valid);
}

TEST(ValidateName, RejectsLeadingNonAlpha) {
    EXPECT_FALSE(validateName("-badstart").valid);
    EXPECT_FALSE(validateName("_badstart").valid);
    EXPECT_FALSE(validateName(".badstart").valid);
}

TEST(ValidateName, RejectsNonPrintable) {
    EXPECT_FALSE(validateName("test\x01").valid);
    EXPECT_FALSE(validateName("test\n").valid);
    EXPECT_FALSE(validateName("test\t").valid);
}

// ========== validateServiceName ==========
TEST(ValidateServiceName, RejectsServerSuffix) {
    EXPECT_FALSE(validateServiceName("my-server").valid);
    EXPECT_FALSE(validateServiceName("prod-server").valid);
}

TEST(ValidateServiceName, AcceptsNonServerSuffix) {
    EXPECT_TRUE(validateServiceName("my-vpn").valid);
    EXPECT_TRUE(validateServiceName("server-main").valid);  // "server" 在开头，不是后缀
}

// ========== validateIPv4 ==========
TEST(ValidateIPv4, AcceptsValidIPs) {
    EXPECT_TRUE(validateIPv4("10.8.0.1").valid);
    EXPECT_TRUE(validateIPv4("192.168.1.1").valid);
    EXPECT_TRUE(validateIPv4("0.0.0.0").valid);
    EXPECT_TRUE(validateIPv4("255.255.255.255").valid);
}

TEST(ValidateIPv4, RejectsOutOfRange) {
    EXPECT_FALSE(validateIPv4("10.8.0.256").valid);
    EXPECT_FALSE(validateIPv4("999.999.999.999").valid);
    EXPECT_FALSE(validateIPv4("-1.0.0.0").valid);
}

TEST(ValidateIPv4, RejectsLeadingZeros) {
    EXPECT_FALSE(validateIPv4("010.008.000.005").valid);
    EXPECT_FALSE(validateIPv4("10.08.0.5").valid);
}

TEST(ValidateIPv4, RejectsMalformed) {
    EXPECT_FALSE(validateIPv4("not-an-ip").valid);
    EXPECT_FALSE(validateIPv4("10.8.0").valid);       // 三组
    EXPECT_FALSE(validateIPv4("10.8.0.1.2").valid);   // 五组
    EXPECT_FALSE(validateIPv4("10.8.0.").valid);      // 尾部点
    EXPECT_FALSE(validateIPv4(".10.8.0.1").valid);    // 首部点
}

// ========== validateSubnet ==========
TEST(ValidateSubnet, AcceptsValidSubnets) {
    EXPECT_TRUE(validateSubnet("10.8.0.0/24").valid);
    EXPECT_TRUE(validateSubnet("172.16.0.0/16").valid);
    EXPECT_TRUE(validateSubnet("192.168.0.0/30").valid);
}

TEST(ValidateSubnet, RejectsNonZeroHostBits) {
    EXPECT_FALSE(validateSubnet("10.8.0.5/24").valid);
    EXPECT_FALSE(validateSubnet("172.16.5.0/16").valid);
}

TEST(ValidateSubnet, RejectsInvalidPrefix) {
    EXPECT_FALSE(validateSubnet("10.8.0.0/0").valid);
    EXPECT_FALSE(validateSubnet("10.8.0.0/7").valid);
    EXPECT_FALSE(validateSubnet("10.8.0.0/31").valid);
    EXPECT_FALSE(validateSubnet("10.8.0.0/33").valid);
}

// ========== validatePort ==========
TEST(ValidatePort, AcceptsValidPorts) {
    EXPECT_TRUE(validatePort(1194).valid);
    EXPECT_TRUE(validatePort(8443).valid);
    EXPECT_TRUE(validatePort(65535).valid);
}

TEST(ValidatePort, RejectsInvalidPorts) {
    EXPECT_FALSE(validatePort(0).valid);
    EXPECT_FALSE(validatePort(80).valid);
    EXPECT_FALSE(validatePort(1023).valid);
    EXPECT_FALSE(validatePort(65536).valid);
    EXPECT_FALSE(validatePort(-1).valid);
}
```

### 15.9 目录结构变化

```
include/
├── Validators.hpp          # 新增 - 参数校验声明
src/
├── Validators.cpp          # 新增 - 参数校验实现
tests/unit/
├── test_validators.cpp     # 新增 - 校验单元测试
```

### 15.10 设计决策总结

| 决策点         | 选择                                           | 理由                                                              |
| -------------- | ---------------------------------------------- | ----------------------------------------------------------------- |
| 校验位置       | SDK 内部 C API 入口处                          | 不依赖调用方自觉，防御性编程                                      |
| 校验方式       | 白名单正则 + 显式禁止字符双重检查              | 即使正则因未知原因被绕过，仍有禁止字符兜底                        |
| 名称字符集     | `[a-zA-Z0-9_.-]`，字母数字开头               | 兼容 OpenVPN 命名惯例，同时排除所有 shell 特殊字符                |
| 服务名后缀约束 | 禁止 `-server` 结尾                          | 避免与 OpenVPN 内部 `systemctl` unit 命名冲突                   |
| IPv4 解析      | 手动实现，不用 `inet_pton`                   | 避免平台差异（Windows 需 Winsock 初始化），且可精确控制前导零检测 |
| 端口范围       | 1024-65535                                     | 避免 Linux 特权端口（<1024 需 root），同时排除 0                  |
| 失败返回       | 返回 `OVPN_ERR_INVALID_PARAM` + 日志输出原因 | 调用方可根据错误码分类处理，日志便于排查                          |
| 性能           | 同步校验，微秒级                               | 校验开销远小于后续的 shell 命令执行（毫秒级），不影响吞吐         |

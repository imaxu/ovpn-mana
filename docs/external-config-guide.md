# 外部配置文件使用指南

## 📋 概述

从 v1.0.2 起，OpenVPN Manager 支持**外部配置文件**，无需重新编译即可适配不同生产环境。

## 🎯 配置优先级

```
环境变量 (最高) > 外部 JSON 文件 > CMake 编译参数 > 代码默认值 (最低)
```

## 📁 配置文件位置

### Linux
```
/etc/openvpn/ovpn-mana.json
```

### Windows
```
<程序目录>\ovpn-mana.json
```

## 🔧 配置文件格式

```json
{
    "easy_rsa_dir": "/etc/openvpn/easy-rsa",
    "ovpn_dir": "/etc/openvpn",
    "openvpn_bin": "/usr/sbin/openvpn",
    "systemctl_bin": "/bin/systemctl"
}
```

### 字段说明

| 字段 | 必填 | 默认值 | 说明 |
|------|------|--------|------|
| `easy_rsa_dir` | 否 | `/etc/openvpn/easy-rsa` | Easy-RSA 工作目录 |
| `ovpn_dir` | 否 | `/etc/openvpn` | OpenVPN 配置根目录 |
| `openvpn_bin` | 否 | `/usr/sbin/openvpn` | openvpn 可执行文件路径 |
| `systemctl_bin` | 否 | `/bin/systemctl` | systemctl 可执行文件路径 |

## 🚀 部署示例

### 场景 1: 标准部署

```bash
# 1. 安装程序
sudo make install

# 2. 编辑配置文件
sudo nano /etc/openvpn/ovpn-mana.json

# 3. 根据实际环境修改
{
    "easy_rsa_dir": "/opt/easy-rsa",
    "ovpn_dir": "/etc/openvpn"
}

# 4. 运行程序（自动读取配置）
sudo openvpnmgr service -l
```

### 场景 2: Docker/K8s 环境变量覆盖

```dockerfile
FROM your-base-image

ENV OVPN_EASY_RSA_DIR=/app/easy-rsa
ENV OVPN_DIR=/app/openvpn

COPY ovpn-mana.json /etc/openvpn/
ENTRYPOINT ["openvpnmgr"]
```

### 场景 3: 多环境配置

**开发环境** (`dev-ovpn-mana.json`):
```json
{
    "easy_rsa_dir": "/home/dev/easy-rsa",
    "ovpn_dir": "/tmp/openvpn-dev"
}
```

**生产环境** (`prod-ovpn-mana.json`):
```json
{
    "easy_rsa_dir": "/data/easy-rsa-prod",
    "ovpn_dir": "/etc/openvpn",
    "openvpn_bin": "/usr/local/sbin/openvpn"
}
```

运行时指定：
```bash
# 使用 API 加载自定义配置
ovpn_mana_load_config(handle, "/path/to/prod-ovpn-mana.json");
```

## 🔌 API 使用

### C/C++ 调用

```c
#include "ovpn-mana/ovpn_mana_api.h"

// 方式 1: 自动加载默认位置配置
ovpn_mana_handle_t handle = ovpn_mana_create();

// 方式 2: 运行时加载自定义配置
ovpn_mana_load_config(handle, "/custom/path/config.json");

// 方式 3: 使用旧版 API 配置（优先级最高）
ovpn_config_t config = {};
strcpy(config.easy_rsa_dir, "/override/path");
ovpn_mana_configure(handle, &config);
```

### 命令行工具

当前版本命令行工具自动读取默认位置配置。如需自定义路径，可通过环境变量：

```bash
export OVPN_EASY_RSA_DIR=/custom/easy-rsa
export OVPN_DIR=/custom/openvpn
sudo openvpnmgr service -l
```

## ✅ 验证配置加载

```bash
# 检查配置是否生效
sudo openvpnmgr --check

# 输出示例：
# [OK] Configuration loaded from /etc/openvpn/ovpn-mana.json
#     Easy-RSA Dir: /opt/easy-rsa
#     OpenVPN Dir: /etc/openvpn
#     OpenVPN Bin: /usr/sbin/openvpn
#     Systemctl Bin: /bin/systemctl
```

## 🛠️ 故障排除

### 配置文件未找到

**症状**: 程序使用默认值而非预期值

**解决方案**:
```bash
# 检查文件是否存在
ls -la /etc/openvpn/ovpn-mana.json

# 从示例复制
sudo cp /usr/local/share/ovpn-mana/ovpn-mana.json.example /etc/openvpn/ovpn-mana.json
sudo nano /etc/openvpn/ovpn-mana.json
```

### JSON 格式错误

**症状**: 程序启动失败或配置未生效

**解决方案**:
```bash
# 验证 JSON 格式
python3 -m json.tool /etc/openvpn/ovpn-mana.json

# 或使用 jq
jq . /etc/openvpn/ovpn-mana.json
```

### 权限问题

**症状**: `Permission denied` 或配置无法读取

**解决方案**:
```bash
# 检查权限
ls -la /etc/openvpn/ovpn-mana.json

# 修正权限（root 可读，其他只读）
sudo chmod 644 /etc/openvpn/ovpn-mana.json
sudo chown root:root /etc/openvpn/ovpn-mana.json
```

## 📊 配置迁移指南

### 从编译时配置迁移

**旧方式** (需要重新编译):
```bash
cmake -DEASY_RSA_DIR=/new/path ..
make && sudo make install
```

**新方式** (修改配置文件即可):
```bash
sudo nano /etc/openvpn/ovpn-mana.json
# 修改 easy_rsa_dir 字段
# 重启服务或重新运行程序
```

### 从硬编码迁移

如果之前在代码中直接修改 [app_config.hpp](../src/core/app_config.hpp) 的默认值，现在可以：

1. 删除代码中的自定义默认值
2. 创建外部配置文件
3. 部署时根据环境调整配置文件

## 🧪 测试验证

运行配置加载器单元测试:

```bash
cd build
ctest -R config_loader -V
```

预期输出:
```
[==========] Running 6 tests from 1 test suite.
[  PASSED  ] 6 tests.
```

## 📝 版本兼容性

- **v1.0.2+**: 完整支持外部配置文件
- **v1.0.2 以前**: 仅支持编译时配置和 API 配置（向后兼容）

## 🔗 相关文档

- [CMake 编译选项](../CMakeLists.txt)
- [API 参考](../include/ovpn-mana/ovpn_mana_api.h)
- [应用配置结构](../src/core/app_config.hpp)
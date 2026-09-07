# 配置文件不生效 - 故障排除指南

## 🚨 症状

```
Executing command: cd /etc/openvpn/easy-rsa && easyrsa --batch build-client-full ...
sh: 1: cd: can't cd to /etc/openvpn/easy-rsa
```

**问题**: 配置文件存在（`/etc/openvpn/ovpn-mana.json`），但程序仍使用默认路径。

---

## 🔍 诊断步骤

### 步骤 1: 运行诊断脚本

```bash
# 在生产环境执行
cd ~/src/openvpn-mana/sdk/build
chmod +x ../../diagnose_config.sh
../../diagnose_config.sh
```

**预期输出示例（正常情况）**:
```
==========================================
OpenVPN Manager 配置诊断工具
==========================================

1. 检查配置文件是否存在...
   ✅ 找到配置文件: /etc/openvpn/ovpn-mana.json

2. 配置文件内容:
{
  "easy_rsa_dir": "/root/easy-rsa",
  ...
}

4. 当前用户:
root

5. 文件可读性检查:
   ✅ 当前用户可读取配置文件

7. 运行程序诊断...
[CONFIG] Loading configuration...
[CONFIG] Successfully opened: /etc/openvpn/ovpn-mana.json
[CONFIG] File size: 123 bytes
[CONFIG] Parsed easy_rsa_dir: /root/easy-rsa
[CONFIG] Configuration loaded successfully from file
Final Configuration:
  easy_rsa_dir:   /root/easy-rsa    ← 应该是你配置的值
```

**异常输出示例（配置未加载）**:
```
[CONFIG] WARNING: Cannot open config file: /etc/openvpn/ovpn-mana.json
[CONFIG] Using default configuration
Final Configuration:
  easy_rsa_dir:   /etc/openvpn/easy-rsa  ← 默认值，说明配置未加载
```

---

## 🛠️ 常见问题及解决方案

### 问题 1: 文件权限不足

**症状**:
```
[CONFIG] WARNING: Cannot open config file: /etc/openvpn/ovpn-mana.json
```

**原因**: 程序运行用户无权读取配置文件

**解决方案**:
```bash
# 检查当前用户
whoami

# 检查文件权限
ls -la /etc/openvpn/ovpn-mana.json

# 修正权限（所有用户可读）
sudo chmod 644 /etc/openvpn/ovpn-mana.json

# 或修改所有者
sudo chown $(whoami) /etc/openvpn/ovpn-mana.json
```

**验证**:
```bash
# 测试可读性
test -r /etc/openvpn/ovpn-mana.json && echo "✅ 可读" || echo "❌ 不可读"
```

---

### 问题 2: JSON 格式错误

**症状**:
```
[CONFIG] Successfully opened: /etc/openvpn/ovpn-mana.json
[CONFIG] File size: 123 bytes
[CONFIG] easy_rsa_dir not found in config, using default
```

**原因**: JSON 格式不正确，解析器无法识别字段

**常见错误**:
```json
// ❌ 错误 1: 使用单引号
{'easy_rsa_dir': '/root/easy-rsa'}

// ❌ 错误 2: 末尾有逗号
{
  "easy_rsa_dir": "/root/easy-rsa",
}

// ❌ 错误 3: 注释（标准 JSON 不支持注释）
{
  "easy_rsa_dir": "/root/easy-rsa"  // 这是注释
}

// ✅ 正确格式
{
  "easy_rsa_dir": "/root/easy-rsa",
  "ovpn_dir": "/etc/openvpn"
}
```

**验证 JSON 格式**:
```bash
# 方法 1: 使用 python
python3 -m json.tool /etc/openvpn/ovpn-mana.json > /dev/null && echo "✅ JSON 格式正确" || echo "❌ JSON 格式错误"

# 方法 2: 使用 jq (如果已安装)
jq . /etc/openvpn/ovpn-mana.json > /dev/null && echo "✅ JSON 格式正确" || echo "❌ JSON 格式错误"

# 方法 3: 在线验证
# 复制内容到 https://jsonlint.com/
```

**修复**:
```bash
# 备份原文件
sudo cp /etc/openvpn/ovpn-mana.json /etc/openvpn/ovpn-mana.json.backup

# 从模板重新创建
sudo tee /etc/openvpn/ovpn-mana.json > /dev/null << 'EOF'
{
  "easy_rsa_dir": "/root/easy-rsa",
  "ovpn_dir": "/etc/openvpn",
  "openvpn_bin": "/usr/sbin/openvpn",
  "systemctl_bin": "/bin/systemctl"
}
EOF
```

---

### 问题 3: 程序使用缓存的旧版本

**症状**: 诊断显示一切正常，但程序仍使用旧路径

**原因**: 动态库被缓存，或运行的是旧版本二进制文件

**解决方案**:
```bash
# 1. 确认运行的程序版本
./openvpnmgr --version
# 或
ldd ./openvpnmgr | grep ovpn-mana

# 2. 清除动态库缓存
sudo ldconfig

# 3. 重新部署（如果是从源码编译）
cd ~/src/openvpn-mana/sdk/build
cmake --build . --config Release
sudo make install

# 4. 重启服务（如果是 systemd 服务）
sudo systemctl restart your-service-name

# 5. 验证加载的库文件路径
ldd ./openvpnmgr | grep ovpn-mana
# 应该显示新编译的库路径，例如:
# libovpn-mana.so => /usr/local/lib/libovpn-mana.so (0x...)
```

---

### 问题 4: 路径中包含特殊字符或空格

**症状**: 配置已加载但命令执行失败

**原因**: 路径包含空格或特殊字符未被正确处理

**示例**:
```json
// ⚠️ 可能有问题
{"easy_rsa_dir": "/path/with spaces/easy-rsa"}

// ✅ 更安全
{"easy_rsa_dir": "/path/without_spaces/easy-rsa"}
```

**验证**:
```bash
# 测试路径是否存在且可访问
cat /etc/openvpn/ovpn-mana.json | grep easy_rsa_dir | awk -F'"' '{print $4}' | xargs ls -la
```

---

### 问题 5: 环境变量覆盖了配置文件

**症状**: 配置文件值正确，但程序使用了其他值

**原因**: 环境变量优先级高于配置文件

**检查**:
```bash
echo "OVPN_EASY_RSA_DIR = ${OVPN_EASY_RSA_DIR:- (未设置)}"
echo "OVPN_DIR = ${OVPN_DIR:- (未设置)}"
```

**临时清除测试**:
```bash
unset OVPN_EASY_RSA_DIR
unset OVPN_DIR
unset OVPN_OPENVPN_BIN
unset OVPN_SYSTEMCTL_BIN

# 重新运行程序
./openvpnmgr --check
```

---

## 📋 完整修复流程

### 场景 A: 首次部署

```bash
# 1. 安装程序
cd ~/src/openvpn-mana/sdk/build
sudo make install
sudo ldconfig

# 2. 部署配置文件
sudo cp ../config/ovpn-mana.json.example /etc/openvpn/ovpn-mana.json

# 3. 编辑配置
sudo nano /etc/openvpn/ovpn-mana.json
# 修改为实际路径

# 4. 设置权限
sudo chmod 644 /etc/openvpn/ovpn-mana.json
sudo chown root:root /etc/openvpn/ovpn-mana.json

# 5. 验证配置
python3 -m json.tool /etc/openvpn/ovpn-mana.json

# 6. 运行诊断
./check_config

# 7. 测试功能
sudo ./openvpnmgr --check
```

### 场景 B: 从旧版本升级

```bash
# 1. 备份旧配置（如果有）
[ -f /etc/openvpn/ovpn-mana.json ] && sudo cp /etc/openvpn/ovpn-mana.json /etc/openvpn/ovpn-mana.json.pre-upgrade

# 2. 编译新版本
cd ~/src/openvpn-mana/sdk/build
git pull origin main
cmake --build . --config Release

# 3. 安装
sudo make install
sudo ldconfig

# 4. 如果没有配置文件，从模板创建
if [ ! -f /etc/openvpn/ovpn-mana.json ]; then
    sudo cp ../config/ovpn-mana.json.example /etc/openvpn/ovpn-mana.json
fi

# 5. 编辑并验证
sudo nano /etc/openvpn/ovpn-mana.json
./check_config

# 6. 重启相关服务
sudo systemctl restart openvpn  # 如果适用
```

---

## 🔬 高级调试

### 启用详细日志

如果上述步骤都无法定位问题，可以启用更详细的日志：

```cpp
// 在 ovpn_mana_api.cpp 的 ovpn_mana_create() 中
// 已经添加了 [CONFIG] 前缀的日志
// 运行时查看 stderr 输出：
./openvpnmgr service -l 2>config_debug.log
cat config_debug.log | grep "\[CONFIG\]"
```

### 使用 strace 跟踪文件访问

```bash
# Linux 下跟踪文件操作
strace -e trace=open,read ./openvpnmgr --check 2>&1 | grep ovpn-mana.json

# 预期输出应包含:
# open("/etc/openvpn/ovpn-mana.json", O_RDONLY) = 3
# 如果没有这行，说明程序根本没有尝试读取配置文件
```

### 检查符号链接

```bash
# 如果使用了符号链接
ls -la /etc/openvpn/ovpn-mana.json
# 确保链接目标存在
file /etc/openvpn/ovpn-mana.json
readlink -f /etc/openvpn/ovpn-mana.json
```

---

## ✅ 验证清单

完成修复后，逐项确认：

- [ ] 配置文件存在于 `/etc/openvpn/ovpn-mana.json`
- [ ] 文件权限允许当前用户读取（`chmod 644`）
- [ ] JSON 格式正确（通过 `json.tool` 或 `jq` 验证）
- [ ] `easy_rsa_dir` 字段值正确指向 Easy-RSA 目录
- [ ] 该目录实际存在：`ls -la <your-easy-rsa-dir>`
- [ ] 运行 `./check_config` 显示正确的路径
- [ ] 运行 `./openvpnmgr --check` 无错误
- [ ] 实际创建客户端功能正常

---

## 📞 快速命令参考

```bash
# 一键诊断
{ cat /etc/openvpn/ovpn-mana.json; echo ""; whoami; ls -la /etc/openvpn/ovpn-mana.json; } | tee /tmp/config_diag.txt

# 一键修复权限
sudo chmod 644 /etc/openvpn/ovpn-mana.json && sudo chown root:root /etc/openvpn/ovpn-mana.json

# 验证 JSON
python3 -m json.tool /etc/openvpn/ovpn-mana.json

# 测试配置加载
./check_config 2>&1 | grep -E "(Easy-RSA|CONFIG|easy_rsa)"

# 强制重新加载库
sudo ldconfig && hash -r
```

---

## 🎯 下一步

如果以上步骤都无法解决问题，请收集以下信息：

1. **完整错误输出**:
   ```bash
   ./openvpnmgr client -c <service>,<name>,<host> 2>&1 | tee error_log.txt
   ```

2. **系统信息**:
   ```bash
   {
     echo "=== OS ==="; cat /etc/os-release;
     echo "=== User ==="; whoami;
     echo "=== Config ==="; cat /etc/openvpn/ovpn-mana.json;
     echo "=== Permissions ==="; ls -la /etc/openvpn/ovpn-mana.json;
     echo "=== Version ==="; ./openvpnmgr --version;
     echo "=== Library ==="; ldd ./openvpnmgr | grep ovpn;
   } | tee system_info.txt
   ```

3. **诊断输出**:
   ```bash
   ./check_config 2>&1 | tee diagnose_output.txt
   ```

将这些文件提交到 Issue，我们将进一步分析。
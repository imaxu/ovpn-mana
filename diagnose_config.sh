#!/bin/bash
# OpenVPN Manager 配置诊断脚本
# 用法: ./diagnose_config.sh

echo "=========================================="
echo "OpenVPN Manager 配置诊断工具"
echo "=========================================="
echo ""

echo "1. 检查配置文件是否存在..."
if [ -f /etc/openvpn/ovpn-mana.json ]; then
    echo "   ✅ 找到配置文件: /etc/openvpn/ovpn-mana.json"
    echo ""
    echo "2. 配置文件内容:"
    cat /etc/openvpn/ovpn-mana.json | head -20
    echo ""
    echo "3. 文件权限:"
    ls -la /etc/openvpn/ovpn-mana.json
    echo ""
    echo "4. 当前用户:"
    whoami
    echo ""
    echo "5. 文件可读性检查:"
    if [ -r /etc/openvpn/ovpn-mana.json ]; then
        echo "   ✅ 当前用户可读取配置文件"
    else
        echo "   ❌ 当前用户无法读取配置文件！"
        echo "   解决方案: sudo chmod 644 /etc/openvpn/ovpn-mana.json"
    fi
else
    echo "   ❌ 未找到配置文件: /etc/openvpn/ovpn-mana.json"
    echo ""
    echo "   可能的原因:"
    echo "   - 安装时未复制配置文件"
    echo "   - 配置文件路径不正确"
    echo ""
    echo "   解决方案:"
    echo "   sudo cp config/ovpn-mana.json.example /etc/openvpn/ovpn-mana.json"
    echo "   sudo nano /etc/openvpn/ovpn-mana.json"
fi

echo ""
echo "6. 检查环境变量..."
if [ -n "$OVPN_EASY_RSA_DIR" ]; then
    echo "   OVPN_EASY_RSA_DIR = $OVPN_EASY_RSA_DIR"
else
    echo "   OVPN_EASY_RSA_DIR = (未设置)"
fi

if [ -n "$OVPN_DIR" ]; then
    echo "   OVPN_DIR = $OVPN_DIR"
else
    echo "   OVPN_DIR = (未设置)"
fi
echo ""

echo "7. 运行程序诊断..."
if [ -f ./check_config ]; then
    ./check_config 2>&1
elif [ -f ./openvpnmgr ]; then
    echo "   使用 openvpnmgr --check 测试:"
    ./openvpnmgr --check 2>&1 | grep -E "\[CONFIG\]|Easy-RSA|easy_rsa"
else
    echo "   ⚠️ 未找到 check_config 或 openvpnmgr"
    echo "   请先编译项目或确保程序在当前目录"
fi

echo ""
echo "=========================================="
echo "诊断完成"
echo "=========================================="
echo ""
echo "如果看到 '[CONFIG] WARNING: Cannot open config file',"
echo "说明程序无法读取配置文件，请检查文件权限。"
echo ""
echo "如果看到 'easy_rsa_dir not found in config',"
echo "说明 JSON 格式可能有误。"
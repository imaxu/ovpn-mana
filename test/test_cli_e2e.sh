#!/bin/bash
set -euo pipefail

BIN="./build/Linux_x86_64/openvpnmgr"
PASS=0
FAIL=0

pass() { echo "  [PASS] $1"; ((PASS++)); }
fail() { echo "  [FAIL] $1"; ((FAIL++)); }

echo "=== P7-5 CLI E2E Test Suite ==="

# ── 辅助函数 ──
svc_name="e2e-test-svc-$(date +%s)"
cli_name="e2e-test-cli-$(date +%s)"
wan_ip="1.2.3.4"
subnet="10.200.0.0/24"
port="12119"
client_ip="10.200.0.100"

cleanup() {
    echo "Cleaning up..."
    $BIN service -d "$svc_name" 2>/dev/null || true
    $BIN client -d "$svc_name" "$cli_name" 2>/dev/null || true
}
trap cleanup EXIT

# ── 1. 服务列表（空状态） ──
echo "1. Service list (empty)"
if $BIN service -l 2>&1 | grep -q "No services"; then
    pass "Empty service list"
else
    fail "Empty service list"
fi

# ── 2. 创建服务 ──
echo "2. Create service"
if $BIN service -c "$svc_name,$port,$subnet" 2>&1; then
    pass "Create service $svc_name"
else
    fail "Create service $svc_name"
fi

# ── 3. 拒绝重复创建 ──
echo "3. Reject duplicate service"
if ! $BIN service -c "$svc_name,$port,$subnet" 2>&1; then
    pass "Reject duplicate service"
else
    fail "Reject duplicate service"
fi

# ── 4. 服务列表（非空） ──
echo "4. Service list (non-empty)"
if $BIN service -l 2>&1 | grep -q "$svc_name"; then
    pass "Service $svc_name in list"
else
    fail "Service $svc_name in list"
fi

# ── 5. 启动服务 ──
echo "5. Start service"
if $BIN service -s "$svc_name" 2>&1; then
    sleep 2
    pass "Start service"
else
    fail "Start service"
fi

# ── 6. 创建客户端（含固定IP） ──
echo "6. Create client with fixed IP"
if $BIN client -c "$svc_name,$cli_name,$wan_ip,$client_ip" 2>&1; then
    pass "Create client $cli_name with fixed IP $client_ip"
else
    fail "Create client $cli_name with fixed IP $client_ip"
fi

# ── 7. 创建客户端（无固定IP） ──
cli_name2="e2e-test-cli2-$(date +%s)"
echo "7. Create client without fixed IP"
if $BIN client -c "$svc_name,$cli_name2,$wan_ip" 2>&1; then
    pass "Create client $cli_name2 without fixed IP"
else
    fail "Create client $cli_name2 without fixed IP"
fi

# ── 8. 导出客户端配置 ──
echo "8. Export client config"
ovpn_file="/tmp/${cli_name}.ovpn"
if $BIN client -export "$svc_name,$cli_name" > "$ovpn_file" 2>&1; then
    if grep -q "remote $wan_ip $port" "$ovpn_file"; then
        pass "Export config contains correct remote"
    else
        fail "Export config missing remote"
    fi
    if grep -q "ifconfig-push $client_ip" "$ovpn_file"; then
        pass "Export config contains fixed IP $client_ip"
    else
        fail "Export config missing fixed IP"
    fi
else
    fail "Export client config"
fi

# ── 9. 在线客户端列表 ──
echo "9. Online clients"
if $BIN service -o "$svc_name" 2>&1; then
    pass "Online clients query"
else
    fail "Online clients query"
fi

# ── 10. 吊销客户端 ──
echo "10. Revoke client"
if $BIN client -d "$svc_name,$cli_name2" 2>&1; then
    pass "Revoke client $cli_name2"
else
    fail "Revoke client $cli_name2"
fi

# ── 11. 停止服务 ──
echo "11. Stop service"
if $BIN service -p "$svc_name" 2>&1; then
    pass "Stop service"
else
    fail "Stop service"
fi

# ── 12. 删除服务 ──
echo "12. Delete service"
if $BIN service -d "$svc_name" 2>&1; then
    pass "Delete service"
else
    fail "Delete service"
fi

# ── 13. 版本命令 ──
echo "13. Version command"
if $BIN --version 2>&1; then
    pass "Version command"
else
    fail "Version command"
fi

# ── 14. 校验拒绝恶意输入 ──
echo "14. Security: reject injection"
if ! $BIN service -c "test;rm -rf /,$port,$subnet" 2>&1; then
    pass "Reject shell injection"
else
    fail "Reject shell injection"
fi

# ── 15. 校验拒绝非法端口 ──
echo "15. Security: reject bad port"
if ! $BIN service -c "test-svc,80,$subnet" 2>&1; then
    pass "Reject port 80"
else
    fail "Reject port 80"
fi

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
#!/usr/bin/env bash
# ============================================================
# 将 explain 放入本地 apt 软件源，实现：sudo apt install explain
# 用法（在 Ubuntu/Debian 上执行，需 root）：
#   sudo ./scripts/setup-apt-repo.sh
# 前置：先运行 ./scripts/build-deb.sh 生成 .deb
# 说明：本地源未签名，使用 [trusted=yes]，仅适合个人/实验环境。
# ============================================================
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$(pwd)"

# ---- 依赖检查 ----
for cmd in apt-ftparchive; do
    command -v "$cmd" >/dev/null 2>&1 || {
        echo "✗ 缺少 $cmd，请先安装：sudo apt install apt-utils"
        exit 1
    }
done

DEB="$(ls -1 "$ROOT"/explain_*_amd64.deb 2>/dev/null | head -n1 || true)"
if [ -z "$DEB" ]; then
    echo "✗ 未找到 .deb 安装包，请先运行：./scripts/build-deb.sh"
    exit 1
fi

REPO_DIR="/srv/apt/explain"
COMP="main/binary-amd64"

echo "==> [1/4] 复制安装包到本地源：$REPO_DIR"
mkdir -p "$REPO_DIR/$COMP"
cp -f "$DEB" "$REPO_DIR/$COMP/"

echo "==> [2/4] 生成 Packages 索引"
cd "$REPO_DIR"
apt-ftparchive packages "$COMP" > "$COMP/Packages"
gzip -9 -n -f "$COMP/Packages"

echo "==> [3/4] 配置 apt 源"
cat > /etc/apt/sources.list.d/explain.list <<EOF
deb [trusted=yes] file:/srv/apt/explain ./
EOF

echo "==> [4/4] 更新 apt 索引"
apt-get update

echo
echo "✅ 本地源已就绪"
echo "安装：sudo apt install explain"
echo "更新（重新打包后）：sudo ./scripts/setup-apt-repo.sh && sudo apt install --only-upgrade explain"
echo "卸载：sudo apt remove explain"

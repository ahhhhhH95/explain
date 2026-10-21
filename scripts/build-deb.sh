#!/usr/bin/env bash
# ============================================================
# 构建 explain 的 Debian 安装包（.deb）
# 用法（在 Ubuntu/Debian 上执行）：
#   ./scripts/build-deb.sh
# 产物：explain_<版本>-1_amd64.deb（位于项目根目录）
# 安装：sudo apt install ./explain_1.1.0-1_amd64.deb
# ============================================================
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$(pwd)"

VERSION="1.1.0"
PKG="explain_${VERSION}-1_amd64.deb"
BUILD_DIR="build-deb"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

# ---- 依赖检查 ----
for cmd in cmake dpkg-deb gzip; do
    command -v "$cmd" >/dev/null 2>&1 || {
        echo "✗ 缺少 $cmd，请先安装构建依赖："
        echo "    sudo apt install cmake g++ libcurl4-openssl-dev"
        exit 1
    }
done

echo "==> [1/4] CMake 构建（Release）"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "==> [2/4] 组装安装内容（staging）"
DESTDIR="$STAGE" cmake --install "$BUILD_DIR"

echo "==> [3/4] 写入 DEBIAN 元数据"
mkdir -p "$STAGE/DEBIAN"
cat > "$STAGE/DEBIAN/control" <<EOF
Package: explain
Version: ${VERSION}-1
Section: utils
Priority: optional
Architecture: amd64
Depends: libcurl4 (>= 7.68.0)
Maintainer: ahhhhh123 <l2006071788@163.com>
Description: Linux 命令智能解释器（大模型 AI 辅助）
 输入 Linux 命令，自动抓取 man 手册并调用大模型 API 生成中文解释，
 适合快速学习、实验报告参考与日常命令检索。
 支持历史记录、Markdown 导出、自定义配置文件（--init/--set/--show-config）
 与 API 连接测试（--ping）。
EOF

# man 手册压缩为 .gz（dpkg 惯例）
gzip -9 -n -f "$STAGE/usr/share/man/man1/explain.1" 2>/dev/null || true

echo "==> [4/4] dpkg-deb 打包"
dpkg-deb --build --root-owner-group "$STAGE" "$PKG"

echo
echo "✅ 生成安装包：$ROOT/$PKG"
echo
echo "一键安装：    sudo apt install ./$PKG"
echo "卸载：        sudo apt remove explain"
echo "查看安装文件：dpkg -L explain"

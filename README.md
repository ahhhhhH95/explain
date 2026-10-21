# explain —— Linux 命令智能解释器（CMake 重构版）

基于豆包大模型（火山方舟）的 Linux 命令解释工具：输入一条命令，程序抓取系统 man 手册原文并交给大模型生成中文解释，支持历史记录、Markdown 导出与用户自定义配置文件。

## 与原始版本的差异（重构要点）

- **工程化**：接口与实现分离（`include/` 头文件 + `src/` 源文件），CMake 构建，`-Wall -Wextra` 编译告警。
- **健壮性修复**：
  1. API Key 不再硬编码，从配置文件或环境变量读取
  2. 修复 popen 命令注入（命令名白名单校验 + `--` 参数隔离）
  3. 默认开启 HTTPS 证书校验（可用 `ssl_verify` 配置关闭）
  4. 用健壮的 JSON 字符串解析器替代脆弱的 find/截断解析（支持全部转义与 `\uXXXX`、代理对）
  5. 检查 HTTP 状态码，错误时给出明确提示
  6. man 手册超长自动截断（`man_max_chars`）
  7. 历史记录改为追加模式，支持 `--last` / `--history`
  8. Markdown 文件名安全化（非法字符替换，防路径穿越）
  9. 管道重定向时自动禁用 ANSI 颜色（`--no-color` 可强制）
  10. curl 请求超时与 User-Agent 设置
- **新功能**：用户自定义配置文件（`~/.explainrc` 或 `--config` 指定），并支持 `--init` 一键生成、`--set` 一键修改配置项、`--show-config` 查看生效配置。

## 构建（Ubuntu）

```bash
sudo apt install cmake g++ libcurl4-openssl-dev
cd explain
cmake -S . -B build
cmake --build build
sudo cmake --install build        # 可选，安装到 /usr/local/bin
```

构建产物：`build/explain`。

## 使用

```bash
./build/explain ls -l                 # 解释命令
./build/explain --last                # 查看最近一次查询
./build/explain --history             # 列出全部历史
./build/explain tar --save            # 解释并导出 tar.md
./build/explain --config /path/to/rc  # 使用指定配置文件
./build/explain --no-color ls         # 强制无颜色输出

# 配置管理（一键生成 / 修改 / 查看）
./build/explain --init                          # 生成 ~/.explainrc（已存在需 --force）
./build/explain --init --force                  # 覆盖已存在的配置文件
./build/explain --init --set api_key=xxx        # 生成后直接写入 api_key
./build/explain --set model=xxx                 # 修改单个配置项
./build/explain --set color=off timeout_sec=60  # 一次修改多个配置项
./build/explain --config myrc --set api_key=xxx # 操作指定配置文件
./build/explain --show-config                   # 查看当前生效配置（Key 打码）

# 连接测试
./build/explain --ping                  # 测试大模型 API 连接（成功显示耗时与模型回复）
./build/explain --ping --config myrc    # 使用指定配置测试
```

说明：
- `--set` 只修改指定项，保留文件中的注释和其余内容；配置项不存在时追加到文件末尾。
- `--set` 会校验配置项名称与取值（布尔项/数值项），写错会直接报错。
- `--init` / `--set` / `--show-config` 可组合使用，执行后立即退出，不会触发 AI 调用。
- `--ping` 发送最小请求验证鉴权、网络与模型可用性：成功输出耗时和模型回复；失败会返回具体原因（未配置 Key、网络错误、HTTP 状态码、API 错误体、响应解析失败等）。
- 等待 API 响应期间，在终端会显示 `| / - \` 旋转动画（输出到 stderr；重定向或非终端时自动隐藏，不污染结果）。

## 配置

首次使用复制模板并编辑：

```bash
cp explainrc.example ~/.explainrc
vi ~/.explainrc    # 填入 api_key
```

配置查找顺序：`--config <文件>` > 环境变量 `EXPLAIN_CONFIG` > `~/.explainrc`；`api_key` 还可由环境变量 `EXPLAIN_API_KEY` 提供。

| 配置项 | 说明 | 默认值 |
|---|---|---|
| `api_key` | API Key（留空则用 `EXPLAIN_API_KEY`） | 无 |
| `api_url` | API 地址 | 火山方舟 chat/completions |
| `model` | 模型名 | doubao-seed-2-1-lite-260915 |
| `history_file` | 历史记录文件 | ~/.explain_history |
| `man_max_chars` | man 文本截断上限（字符） | 4000 |
| `timeout_sec` | HTTP 超时（秒） | 30 |
| `color` | ANSI 颜色开关 | true |
| `ssl_verify` | HTTPS 证书校验 | true |

## 项目结构

```
explain/
├── CMakeLists.txt
├── README.md
├── explainrc.example
├── include/explain/     # 接口声明
│   ├── config.h         # 配置结构体与加载
│   ├── history.h        # 历史记录
│   ├── http.h           # 豆包 API 调用
│   ├── manpage.h        # man 手册获取与命令推荐
│   ├── output.h         # 格式化输出
│   └── util.h           # 通用工具（JSON/字符串/校验）
└── src/                 # 实现
    ├── main.cpp         # 入口与参数解析
    ├── config.cpp
    ├── history.cpp
    ├── http.cpp
    ├── manpage.cpp
    ├── output.cpp
    └── util.cpp
```

## 已知限制

- man 手册版本差异可能导致个别命令无手册
- 未做请求并发/重试（可配合 `timeout_sec` 使用）
- 配置文件为轻量 INI 风格，不支持引号包裹的多行值

#include "explain/output.h"
#include "explain/util.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace explain {

namespace {

const char *COL_TITLE = "\033[1;34m";
const char *COL_KEY = "\033[1;32m";
const char *COL_TEXT = "\033[0;37m";
const char *COL_WARN = "\033[1;31m";
const char *COL_RESET = "\033[0m";

} // namespace

void printFormatted(const std::string &text, bool color)
{
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("【") != std::string::npos) {
            std::cout << (color ? COL_TITLE : "") << line << (color ? COL_RESET : "") << "\n";
        } else if (line.rfind("-", 0) == 0 || line.rfind("  -", 0) == 0) {
            std::cout << (color ? COL_KEY : "") << line << (color ? COL_RESET : "") << "\n";
        } else {
            std::cout << (color ? COL_TEXT : "") << line << (color ? COL_RESET : "") << "\n";
        }
    }
}

void saveMarkdown(const std::string &cmdline, const std::string &text)
{
    std::string fname = sanitizeFilename(cmdline);
    std::ofstream f(fname + ".md");
    if (!f.is_open()) {
        std::cerr << "❌ 无法写入 " << fname << ".md\n";
        return;
    }
    f << "# Linux 命令解释：" << cmdline << "\n\n" << text << "\n";
    f.close();
    std::cout << "✅ 已保存为 " << fname << ".md\n";
}

void printUsage()
{
    std::cout <<
        "explain —— Linux 命令智能解释器（基于豆包大模型）\n\n"
        "用法:\n"
        "  explain [选项] <命令...>\n"
        "  explain --last            查看最近一次查询\n"
        "  explain --history         列出全部历史记录\n"
        "  explain ls -l --save      解释命令并导出 Markdown 笔记\n"
        "  explain --init [--force]  一键生成配置文件（默认 ~/.explainrc）\n"
        "  explain --set key=value   一键修改/新增配置项（可多次使用）\n"
        "  explain --show-config     查看当前生效配置\n\n"
        "选项:\n"
        "  -h, --help           显示本帮助\n"
        "  --config <文件>      指定配置文件（默认依次查找 $EXPLAIN_CONFIG、~/.explainrc）\n"
        "  --init               生成配置文件，已存在时需加 --force 覆盖\n"
        "  --force              配合 --init 覆盖已存在的配置文件\n"
        "  --set key=value      修改或新增配置项，可重复使用，如 --set model=xxx\n"
        "  --show-config        显示当前生效的配置（API Key 打码显示）\n"
        "  --no-color           禁用 ANSI 彩色输出\n"
        "  --save               把结果保存为 <命令>.md\n"
        "  --last               显示最近一次查询\n"
        "  --history            列出全部历史记录\n\n"
        "配置文件字段（每行 键 = 值，# 开头为注释）:\n"
        "  api_key        API Key（也可用环境变量 EXPLAIN_API_KEY）\n"
        "  api_url        API 地址（默认: https://ark.cn-beijing.volces.com/api/v3/chat/completions）\n"
        "  model          模型名（默认: doubao-seed-2-1-lite-260915）\n"
        "  history_file   历史记录路径（默认: ~/.explain_history）\n"
        "  man_max_chars  man 手册送入模型的截断上限，字符数（默认: 4000）\n"
        "  timeout_sec    HTTP 超时秒数（默认: 30）\n"
        "  color          true/false，是否输出 ANSI 颜色（默认: true）\n"
        "  ssl_verify     true/false，是否校验 HTTPS 证书（默认: true）\n";
}

void printError(const std::string &msg, bool color)
{
    std::cerr << (color ? COL_WARN : "") << msg << (color ? COL_RESET : "") << "\n";
}

} // namespace explain

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <curl/curl.h>

#include "explain/config.h"
#include "explain/history.h"
#include "explain/http.h"
#include "explain/manpage.h"
#include "explain/output.h"
#include "explain/util.h"

namespace {

// RAII 封装 curl 全局初始化，确保每个退出路径都执行 cleanup
struct CurlGuard {
    CurlGuard() { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlGuard() { curl_global_cleanup(); }
};

struct Options {
    std::string configPath;
    bool showLast = false;
    bool showHistory = false;
    bool save = false;
    bool noColor = false;
    bool help = false;
    std::vector<std::string> args; // 命令词
};

bool parseOptions(int argc, char *argv[], Options &opt, std::string &err)
{
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--last") opt.showLast = true;
        else if (a == "--history") opt.showHistory = true;
        else if (a == "--save") opt.save = true;
        else if (a == "--no-color") opt.noColor = true;
        else if (a == "-h" || a == "--help") opt.help = true;
        else if (a == "--config") {
            if (i + 1 >= argc) {
                err = "--config 后缺少文件路径";
                return false;
            }
            opt.configPath = argv[++i];
        } else if (a.rfind("--config=", 0) == 0) {
            opt.configPath = a.substr(9);
            if (opt.configPath.empty()) {
                err = "--config 后缺少文件路径";
                return false;
            }
        } else if (a.size() >= 2 && a[0] == '-' && a[1] == '-') {
            err = "未知选项: " + a;
            return false;
        } else {
            opt.args.push_back(a);
        }
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    using namespace explain;

    CurlGuard curlGuard;

    Options opt;
    std::string perr;
    if (!parseOptions(argc, argv, opt, perr)) {
        printError(perr, true);
        return 2;
    }
    if (opt.help) {
        printUsage();
        return 0;
    }

    // 颜色：默认按终端自动检测，--no-color 强制关闭
    bool useColor = opt.noColor ? false : stdoutIsTty();

    // 加载配置（默认值 < 配置文件 < 环境变量）
    std::string cerr;
    Config cfg = loadConfig(opt.configPath, cerr);
    if (!cerr.empty()) printError(cerr, useColor); // 显式指定的配置文件不存在等情况
    if (cfg.configLoaded) {
        std::cout << "配置文件: " << cfg.configPath << "\n";
    }

    // 历史查询分支
    if (opt.showLast || opt.showHistory) {
        auto entries = loadHistory(cfg.historyFile);
        if (entries.empty()) {
            std::cout << "暂无历史记录（历史文件: " << cfg.historyFile << "）\n";
            return 0;
        }
        if (opt.showLast) {
            const HistoryEntry &e = entries.back();
            std::cout << "===== 上次查询【" << e.cmd << "】=====\n";
            printFormatted(e.content, useColor);
        } else {
            std::cout << "===== 历史记录（共 " << entries.size() << " 条）=====\n";
            for (std::size_t i = 0; i < entries.size(); ++i) {
                std::cout << "[" << (i + 1) << "] " << entries[i].cmd << "\n";
            }
            std::cout << "（用 --last 查看最近一次的完整内容）\n";
        }
        return 0;
    }

    // 无命令词 → 打印使用说明
    if (opt.args.empty()) {
        printUsage();
        return 0;
    }

    // 拼装完整命令行（用于发给 AI 与保存历史）
    std::string cmdLine;
    for (const auto &s : opt.args) cmdLine += s + " ";
    if (!cmdLine.empty()) cmdLine.pop_back();

    // 1. 获取 man 手册（含命令名白名单校验与截断）
    ManResult man = getManPage(opt.args[0], cfg.manMaxChars);
    if (!man.ok) {
        printError("❌ " + man.err, useColor);
        std::string best = suggestCommand(opt.args[0]);
        if (!best.empty()) {
            std::cout << "是否输入错误？相近命令推荐: " << best << "\n";
        }
        return 1;
    }

    // 2. 调用豆包 API
    std::cout << "正在调用 AI 解析命令，请稍候...\n";
    AiResult ai = callAI(cfg, man.text, cmdLine);
    if (!ai.ok) {
        printError("❌ " + ai.err, useColor);
        return 1;
    }

    // 3. 保存历史并输出
    saveHistory(cfg.historyFile, cmdLine, ai.content);
    printFormatted(ai.content, useColor);

    if (opt.save) saveMarkdown(cmdLine, ai.content);
    return 0;
}

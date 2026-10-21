#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
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
    bool init = false;
    bool force = false;
    bool showConfig = false;
    bool ping = false;
    std::vector<std::pair<std::string, std::string>> sets; // --set key=value
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
        else if (a == "--init") opt.init = true;
        else if (a == "--force") opt.force = true;
        else if (a == "--show-config") opt.showConfig = true;
        else if (a == "--ping") opt.ping = true;
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
        } else if (a == "--set") {
            if (i + 1 >= argc) {
                err = "--set 后缺少 key=value";
                return false;
            }
            std::string kv = argv[++i];
            std::size_t eq = kv.find('=');
            if (eq == std::string::npos || eq == 0) {
                err = "--set 参数格式应为 key=value";
                return false;
            }
            opt.sets.emplace_back(kv.substr(0, eq), kv.substr(eq + 1));
        } else if (a.rfind("--set=", 0) == 0) {
            std::string kv = a.substr(6);
            std::size_t eq = kv.find('=');
            if (eq == std::string::npos || eq == 0) {
                err = "--set 参数格式应为 key=value";
                return false;
            }
            opt.sets.emplace_back(kv.substr(0, eq), kv.substr(eq + 1));
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
    // 配置管理场景（--init/--set/--show-config）下，配置文件不存在属正常流程
    // （文件随后会被创建），不打印该警告；其余场景打印
    bool cfgManage = opt.init || !opt.sets.empty() || opt.showConfig;
    if (!cfgManage && !cerr.empty()) printError(cerr, useColor);
    if (!cfgManage && cfg.configLoaded) {
        std::cout << "配置文件: " << cfg.configPath << "\n";
    }

    // 配置文件管理分支：--init / --set / --show-config
    if (cfgManage) {
        // 确定操作目标路径：--config 指定 > 已加载的配置文件 > 默认 ~/.explainrc
        std::string target = opt.configPath;
        if (target.empty()) target = cfg.configPath;
        if (target.empty()) target = expandHome("~/.explainrc");

        if (opt.init) {
            std::string ierr;
            if (initConfigFile(target, opt.force, ierr)) {
                std::cout << "✅ 已生成配置文件: " << target << "\n";
            } else {
                printError("❌ " + ierr, useColor);
            }
        }
        if (!opt.sets.empty()) {
            std::string serr;
            if (setConfigValue(target, opt.sets, serr)) {
                std::cout << "✅ 已更新配置文件: " << target << "\n";
                for (const auto &kv : opt.sets) {
                    std::cout << "   " << kv.first << " = " << kv.second << "\n";
                }
            } else {
                printError("❌ " + serr, useColor);
            }
        }

        // 配置管理操作后重新加载，让 --show-config 显示最新结果
        if (opt.init || !opt.sets.empty()) {
            std::string rerr;
            cfg = loadConfig(opt.configPath, rerr);
        }

        if (opt.showConfig) {
            std::cout << "===== 当前配置 =====" << "\n";
            std::cout << "配置文件路径: " << (cfg.configLoaded ? cfg.configPath : "（未加载，使用内置默认值）") << "\n";
            std::cout << "api_key      : " << (cfg.apiKey.empty() ? "（未设置）" : cfg.apiKey.substr(0, 4) + "****") << "\n";
            std::cout << "api_url      : " << cfg.apiUrl << "\n";
            std::cout << "model        : " << cfg.model << "\n";
            std::cout << "history_file : " << cfg.historyFile << "\n";
            std::cout << "man_max_chars: " << cfg.manMaxChars << "\n";
            std::cout << "timeout_sec  : " << cfg.timeoutSec << "\n";
            std::cout << "color        : " << (cfg.color ? "true" : "false") << "\n";
            std::cout << "ssl_verify   : " << (cfg.sslVerify ? "true" : "false") << "\n";
        }
        return 0;
    }

    // 连接测试分支：--ping（确认大模型 API 是否可用，失败返回具体报错）
    if (opt.ping) {
        using Clock = std::chrono::steady_clock;
        auto t0 = Clock::now();
        AiResult r = testConnection(cfg);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
        if (r.ok) {
            std::cout << "✅ API 连接成功（耗时 " << ms << " ms，模型: " << cfg.model << "）\n";
            std::cout << "模型回复: " << r.content << "\n";
            return 0;
        }
        printError("❌ API 连接失败（耗时 " + std::to_string(ms) + " ms）: " + r.err, useColor);
        return 1;
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

    // 2. 调用豆包 API（等待期间 stderr 显示旋转动画）
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

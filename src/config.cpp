#include "explain/config.h"
#include "explain/util.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace explain {

namespace {

const char *kDefaultUrl = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";
const char *kDefaultModel = "doubao-seed-2-1-lite-260915";
const char *kDefaultHistory = "~/.explain_history";
const char *kEnvConfig = "EXPLAIN_CONFIG";
const char *kEnvApiKey = "EXPLAIN_API_KEY";

bool parseBool(const std::string &v, bool def, std::string &warn)
{
    std::string low;
    for (char c : trim(v)) low += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (low == "true" || low == "yes" || low == "on" || low == "1") return true;
    if (low == "false" || low == "no" || low == "off" || low == "0") return false;
    warn = "无法识别的布尔值 '" + v + "'，保持原值";
    return def;
}

void parseConfigFile(const std::string &path, Config &cfg)
{
    std::string content;
    if (!readFile(path, content)) return; // 读不到由调用方判断（configLoaded 保持 false）
    std::istringstream iss(content);
    std::string line;
    int lineno = 0;
    while (std::getline(iss, line)) {
        ++lineno;
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        // 行内注释：前面带空格的 #
        std::size_t hash = t.find(" #");
        if (hash != std::string::npos) t = trim(t.substr(0, hash));
        if (t.empty()) continue;
        std::size_t eq = t.find('=');
        if (eq == std::string::npos) {
            std::cerr << "[warn] " << path << ":" << lineno << " 无法解析（缺少 =）: " << t << "\n";
            continue;
        }
        std::string key = trim(t.substr(0, eq));
        std::string val = trim(t.substr(eq + 1));

        if (key == "api_key") {
            if (!val.empty()) cfg.apiKey = val;
        } else if (key == "api_url") {
            if (!val.empty()) cfg.apiUrl = val;
        } else if (key == "model") {
            if (!val.empty()) cfg.model = val;
        } else if (key == "history_file") {
            if (!val.empty()) cfg.historyFile = expandHome(val);
        } else if (key == "man_max_chars") {
            char *end = nullptr;
            unsigned long long n = std::strtoull(val.c_str(), &end, 10);
            if (end != val.c_str() && n > 0) {
                cfg.manMaxChars = static_cast<std::size_t>(n);
            } else {
                std::cerr << "[warn] " << path << ":" << lineno << " man_max_chars 非法: " << val << "\n";
            }
        } else if (key == "timeout_sec") {
            char *end = nullptr;
            long n = std::strtol(val.c_str(), &end, 10);
            if (end != val.c_str() && n > 0) {
                cfg.timeoutSec = n;
            } else {
                std::cerr << "[warn] " << path << ":" << lineno << " timeout_sec 非法: " << val << "\n";
            }
        } else if (key == "color") {
            std::string w;
            cfg.color = parseBool(val, cfg.color, w);
            if (!w.empty()) std::cerr << "[warn] " << path << ":" << lineno << " " << w << "\n";
        } else if (key == "ssl_verify") {
            std::string w;
            cfg.sslVerify = parseBool(val, cfg.sslVerify, w);
            if (!w.empty()) std::cerr << "[warn] " << path << ":" << lineno << " " << w << "\n";
        } else {
            std::cerr << "[warn] " << path << ":" << lineno << " 未知配置项: " << key << "\n";
        }
    }
    cfg.configLoaded = true;
    cfg.configPath = path;
}

} // namespace

Config loadConfig(const std::string &explicitPath, std::string &err)
{
    Config cfg;
    cfg.apiUrl = kDefaultUrl;
    cfg.model = kDefaultModel;
    cfg.historyFile = expandHome(kDefaultHistory);

    std::string path = explicitPath;
    bool explicitFile = !explicitPath.empty();
    if (!explicitFile) {
        const char *env = std::getenv(kEnvConfig);
        if (env && *env) path = env;
        else path = expandHome("~/.explainrc");
    }

    if (path.empty()) {
        err = "配置文件路径为空";
        return cfg;
    }
    parseConfigFile(path, cfg);
    if (!cfg.configLoaded && explicitFile) {
        err = "指定的配置文件不存在或无法读取: " + path;
    }
    // 默认查找链没找到文件 → 正常使用内置默认值，不报错

    // 环境变量覆盖（显式提供时优先级最高）
    const char *ak = std::getenv(kEnvApiKey);
    if (ak && *ak) cfg.apiKey = ak;

    return cfg;
}

std::string configTemplate()
{
    return
        "# explain 配置文件示例\n"
        "# 用法：cp explainrc.example ~/.explainrc 后按需修改\n"
        "\n"
        "# API Key：可留空，改用环境变量 EXPLAIN_API_KEY 提供\n"
        "api_key = \n"
        "\n"
        "# API 地址与模型（一般无需修改）\n"
        "api_url = https://ark.cn-beijing.volces.com/api/v3/chat/completions\n"
        "model = doubao-seed-2-1-lite-260915\n"
        "\n"
        "# 历史记录文件路径\n"
        "history_file = ~/.explain_history\n"
        "\n"
        "# man 手册送入模型的截断上限（字符数，避免超出模型上下文）\n"
        "man_max_chars = 4000\n"
        "\n"
        "# HTTP 超时（秒）\n"
        "timeout_sec = 30\n"
        "\n"
        "# 是否输出 ANSI 颜色\n"
        "color = true\n"
        "\n"
        "# 是否校验 HTTPS 证书（网络环境无有效 CA 时改为 false）\n"
        "ssl_verify = true\n";
}

} // namespace explain

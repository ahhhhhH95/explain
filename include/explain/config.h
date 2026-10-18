#ifndef EXPLAIN_CONFIG_H
#define EXPLAIN_CONFIG_H

#include <cstddef>
#include <string>

namespace explain {

struct Config {
    std::string apiKey;               // API Key（空 = 未配置）
    std::string apiUrl;               // API 地址
    std::string model;                // 模型名
    std::string historyFile;          // 历史记录文件路径
    std::size_t manMaxChars = 4000;   // man 手册送入模型的截断上限（字符数）
    long timeoutSec = 30;             // HTTP 超时（秒）
    bool color = true;                // 是否输出 ANSI 颜色
    bool sslVerify = true;            // 是否校验 HTTPS 证书

    bool configLoaded = false;        // 是否读到了配置文件
    std::string configPath;           // 实际使用的配置文件路径
};

// 加载配置。优先级：内置默认值 < 配置文件 < 环境变量。
// explicitPath 非空时强制使用该文件（不存在时 configLoaded=false 并在 err 说明）；
// 否则依次尝试 $EXPLAIN_CONFIG、~/.explainrc。
// api_key 额外支持环境变量 EXPLAIN_API_KEY 覆盖。
Config loadConfig(const std::string &explicitPath, std::string &err);

// 返回配置文件模板内容（可写入 explainrc.example）
std::string configTemplate();

} // namespace explain

#endif // EXPLAIN_CONFIG_H

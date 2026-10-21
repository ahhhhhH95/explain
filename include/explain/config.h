#ifndef EXPLAIN_CONFIG_H
#define EXPLAIN_CONFIG_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

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

// 一键生成配置文件（内容为模板）。force=false 且文件已存在时返回 false 并在 err 说明。
bool initConfigFile(const std::string &path, bool force, std::string &err);

// 一键修改/新增配置项并写回。文件不存在时先按模板创建。
// updates 中 key 必须是已知配置项，value 会做基本合法性校验；
// 已存在的键原位更新（保留其余内容与注释），新键追加到文件末尾。
bool setConfigValue(const std::string &path,
                    const std::vector<std::pair<std::string, std::string>> &updates,
                    std::string &err);

} // namespace explain

#endif // EXPLAIN_CONFIG_H

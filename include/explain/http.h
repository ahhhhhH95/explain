#ifndef EXPLAIN_HTTP_H
#define EXPLAIN_HTTP_H

#include <string>

#include "explain/config.h"

namespace explain {

struct AiResult {
    bool ok = false;
    std::string content;  // AI 回答
    std::string err;      // 错误信息
};

// 调用豆包大模型 API：以 man 手册原文为上下文，返回命令的中文解释。
// 使用配置中的 apiKey / apiUrl / model / timeoutSec / sslVerify。
AiResult callAI(const Config &cfg, const std::string &manText, const std::string &cmdline);

// 测试大模型 API 连接是否成功（发送最小请求）。
// 成功时 ok=true 且 content 为模型回复；失败时 err 为具体报错（HTTP 状态码/错误体等）。
AiResult testConnection(const Config &cfg);

} // namespace explain

#endif // EXPLAIN_HTTP_H

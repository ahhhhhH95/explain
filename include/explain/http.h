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

} // namespace explain

#endif // EXPLAIN_HTTP_H

#ifndef EXPLAIN_MANPAGE_H
#define EXPLAIN_MANPAGE_H

#include <cstddef>
#include <string>
#include <vector>

namespace explain {

struct ManResult {
    bool ok = false;
    std::string text;   // 手册纯文本（可能被截断）
    std::string err;    // 失败原因
};

// 获取命令手册纯文本。cmd 需通过 isValidCmdName 校验；
// maxChars > 0 时超长截断。
ManResult getManPage(const std::string &cmd, std::size_t maxChars);

// 编辑距离（Levenshtein），用于命令纠错
int editDistance(const std::string &a, const std::string &b);

// 在内置常用命令中推荐最接近的一个
std::string suggestCommand(const std::string &cmd);

} // namespace explain

#endif // EXPLAIN_MANPAGE_H

#ifndef EXPLAIN_OUTPUT_H
#define EXPLAIN_OUTPUT_H

#include <string>

namespace explain {

// 按行格式化输出：含【】的标题行用标题色，- 开头的高亮行用强调色，其余为正文。
// color=false 时不输出任何 ANSI 转义序列。
void printFormatted(const std::string &text, bool color);

// 将结果保存为 Markdown 文件（<安全化命令名>.md，位于当前目录）
void saveMarkdown(const std::string &cmdline, const std::string &text);

// 打印使用说明（stdout）
void printUsage();

// 打印错误信息（stderr，红色）
void printError(const std::string &msg, bool color);

} // namespace explain

#endif // EXPLAIN_OUTPUT_H

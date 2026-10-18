#ifndef EXPLAIN_UTIL_H
#define EXPLAIN_UTIL_H

#include <string>

namespace explain {

// JSON 字符串值转义（用于构造请求体）
std::string jsonEscape(const std::string &s);

// 从 JSON 文本中提取 key 对应的字符串字面量值，
// 正确处理 \" \\ \/ \b \f \n \r \t \uXXXX（含代理对）。
// 找到并成功解析返回 true，out 为解析后的内容。
bool extractJsonStringValue(const std::string &json, const std::string &key, std::string &out);

// 去除首尾空白
std::string trim(const std::string &s);

// 校验命令名是否只含安全字符（防命令注入）。
// 仅允许字母、数字、_ - . /，且不能以 - 开头。
bool isValidCmdName(const std::string &s);

// 将文件名中的不安全字符替换为 '_'，防止路径穿越与特殊字符问题
std::string sanitizeFilename(const std::string &s);

// 展开开头的 ~ 为用户主目录（仅支持 ~ 和 ~/ 形式）
std::string expandHome(const std::string &path);

// 读取整个文件内容（二进制方式）
bool readFile(const std::string &path, std::string &out);

// 标准输出是否为终端（用于决定是否输出 ANSI 颜色）
bool stdoutIsTty();

} // namespace explain

#endif // EXPLAIN_UTIL_H

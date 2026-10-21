#include "explain/util.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <io.h>
#define EXPLAIN_ISATTY _isatty
#define EXPLAIN_FILENO _fileno
#else
#include <unistd.h>
#define EXPLAIN_ISATTY isatty
#define EXPLAIN_FILENO fileno
#endif

namespace explain {

std::string jsonEscape(const std::string &s)
{
    std::string res;
    char buf[8];
    for (char c : s) {
        switch (c) {
            case '"': res += "\\\""; break;
            case '\\': res += "\\\\"; break;
            case '\n': res += "\\n"; break;
            case '\r': res += "\\r"; break;
            case '\t': res += "\\t"; break;
            case '\b': res += "\\b"; break;
            case '\f': res += "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    res += buf;
                } else {
                    res += c;
                }
        }
    }
    return res;
}

namespace {

// 从 json[pos]（应为 '"'）解析一个 JSON 字符串字面量，支持全部标准转义
bool parseJsonStringLiteral(const std::string &json, std::size_t pos, std::string &out)
{
    if (pos >= json.size() || json[pos] != '"') return false;
    std::size_t i = pos + 1;
    out.clear();
    while (i < json.size()) {
        char c = json[i];
        if (c == '"') return true;
        if (c == '\\') {
            if (i + 1 >= json.size()) return false;
            char e = json[i + 1];
            switch (e) {
                case '"': out += '"'; i += 2; break;
                case '\\': out += '\\'; i += 2; break;
                case '/': out += '/'; i += 2; break;
                case 'n': out += '\n'; i += 2; break;
                case 'r': out += '\r'; i += 2; break;
                case 't': out += '\t'; i += 2; break;
                case 'b': out += '\b'; i += 2; break;
                case 'f': out += '\f'; i += 2; break;
                case 'u': {
                    auto hex4 = [&](std::size_t at, unsigned &v) -> bool {
                        if (at + 4 > json.size()) return false;
                        v = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = json[at + k];
                            v <<= 4;
                            if (h >= '0' && h <= '9') v |= static_cast<unsigned>(h - '0');
                            else if (h >= 'a' && h <= 'f') v |= static_cast<unsigned>(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') v |= static_cast<unsigned>(h - 'A' + 10);
                            else return false;
                        }
                        return true;
                    };
                    unsigned cp = 0;
                    if (!hex4(i + 2, cp)) return false;
                    i += 6;
                    // 代理对（高代理后紧跟 \u 低代理）
                    if (cp >= 0xD800 && cp <= 0xDBFF &&
                        i + 2 <= json.size() && json[i] == '\\' && json[i + 1] == 'u') {
                        unsigned lo = 0;
                        if (hex4(i + 2, lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            i += 6;
                        }
                    }
                    // 编码为 UTF-8
                    if (cp < 0x80) {
                        out += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        out += static_cast<char>(0xC0 | (cp >> 6));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000) {
                        out += static_cast<char>(0xE0 | (cp >> 12));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        out += static_cast<char>(0xF0 | (cp >> 18));
                        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default:
                    return false; // 非法转义
            }
        } else {
            if (static_cast<unsigned char>(c) < 0x20) return false; // 未转义的控制字符
            out += c;
            ++i;
        }
    }
    return false; // 字符串未闭合
}

} // namespace

bool extractJsonStringValue(const std::string &json, const std::string &key, std::string &out)
{
    std::string pat = "\"" + key + "\"";
    std::size_t pos = 0;
    while ((pos = json.find(pat, pos)) != std::string::npos) {
        std::size_t p = pos + pat.size();
        while (p < json.size() &&
               (json[p] == ' ' || json[p] == '\t' || json[p] == '\r' || json[p] == '\n')) {
            ++p;
        }
        if (p < json.size() && json[p] == ':') {
            ++p;
            while (p < json.size() &&
                   (json[p] == ' ' || json[p] == '\t' || json[p] == '\r' || json[p] == '\n')) {
                ++p;
            }
            if (p < json.size() && json[p] == '"') {
                if (parseJsonStringLiteral(json, p, out)) return true;
            }
        }
        pos += pat.size();
    }
    return false;
}

std::string trim(const std::string &s)
{
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

bool isValidCmdName(const std::string &s)
{
    if (s.empty() || s[0] == '-') return false;
    for (char c : s) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || c == '_' || c == '-' || c == '.' || c == '/') continue;
        return false;
    }
    return true;
}

std::string sanitizeFilename(const std::string &s)
{
    std::string r;
    for (char c : s) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || c == '_' || c == '-' || c == '.') r += c;
        else r += '_';
    }
    if (r.empty() || r == "." || r == "..") r = "explain";
    return r;
}

std::string expandHome(const std::string &path)
{
    if (path.empty() || path[0] != '~') return path;
    const char *home = std::getenv("HOME");
    std::string h = home ? std::string(home) : "";
    if (path == "~") return h.empty() ? path : h;
    if (path.size() >= 2 && path[1] == '/') {
        return h.empty() ? path : h + path.substr(1);
    }
    return path; // ~user 形式不支持，原样返回
}

bool readFile(const std::string &path, std::string &out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

bool stdoutIsTty()
{
    return EXPLAIN_ISATTY(EXPLAIN_FILENO(stdout)) != 0;
}

bool stderrIsTty()
{
    return EXPLAIN_ISATTY(EXPLAIN_FILENO(stderr)) != 0;
}

} // namespace explain

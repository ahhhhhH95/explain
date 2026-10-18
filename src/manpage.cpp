#include "explain/manpage.h"
#include "explain/util.h"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace explain {

namespace {

const char *kCommonCommands[] = {
    "ls", "cp", "mv", "rm", "cat", "grep", "chmod", "chown", "tar", "find",
    "pwd", "cd", "mkdir", "rmdir", "touch", "ps", "top", "kill", "df", "du",
    "ping", "ifconfig", "ip", "systemctl", "apt", "ssh", "scp", "sed", "awk", "head"
};

} // namespace

ManResult getManPage(const std::string &cmd, std::size_t maxChars)
{
    ManResult res;
    if (!isValidCmdName(cmd)) {
        res.err = "非法命令名（仅允许字母、数字、_ - . / 且不能以 - 开头）";
        return res;
    }
    // 使用 "--" 隔离命令名，防止以 "-" 开头的名字被 man 当作选项
    std::string shellCmd = "man -P cat -- " + cmd + " 2>/dev/null";
    FILE *fp = popen(shellCmd.c_str(), "r");
    if (!fp) {
        res.err = "popen 失败，无法调用 man";
        return res;
    }
    char buf[4096];
    std::string text;
    while (fgets(buf, sizeof(buf), fp) != nullptr) text += buf;
    int rc = pclose(fp);
    (void)rc;

    if (text.empty()) {
        res.err = "系统手册中未找到命令 '" + cmd + "'";
        return res;
    }
    if (maxChars > 0 && text.size() > maxChars) text.resize(maxChars);
    res.ok = true;
    res.text = text;
    return res;
}

int editDistance(const std::string &a, const std::string &b)
{
    int n = static_cast<int>(a.size()), m = static_cast<int>(b.size());
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1));
    for (int i = 0; i <= n; ++i) dp[i][0] = i;
    for (int j = 0; j <= m; ++j) dp[0][j] = j;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1]) dp[i][j] = dp[i - 1][j - 1];
            else dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
        }
    }
    return dp[n][m];
}

std::string suggestCommand(const std::string &cmd)
{
    std::string best;
    int minD = 1000;
    for (const char *c : kCommonCommands) {
        int d = editDistance(cmd, c);
        if (d < minD) {
            minD = d;
            best = c;
        }
    }
    return best;
}

} // namespace explain

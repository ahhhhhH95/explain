#ifndef EXPLAIN_HISTORY_H
#define EXPLAIN_HISTORY_H

#include <string>
#include <vector>

namespace explain {

struct HistoryEntry {
    std::string cmd;
    std::string content;
};

// 追加一条查询记录（追加模式，保留全部历史）
bool saveHistory(const std::string &historyFile, const std::string &cmd, const std::string &content);

// 读取全部历史记录（文件不存在返回空列表）
std::vector<HistoryEntry> loadHistory(const std::string &historyFile);

// 读取最近一条记录
bool loadLast(const std::string &historyFile, HistoryEntry &out);

} // namespace explain

#endif // EXPLAIN_HISTORY_H

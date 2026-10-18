#include "explain/history.h"

#include <fstream>

namespace explain {

namespace {

const char *kMarkCmd = "====CMD====";
const char *kMarkSplit = "====SPLIT====";
const char *kMarkEnd = "====END====";

} // namespace

bool saveHistory(const std::string &historyFile, const std::string &cmd, const std::string &content)
{
    std::ofstream f(historyFile, std::ios::app);
    if (!f.is_open()) return false;
    f << kMarkCmd << "\n" << cmd << "\n"
      << kMarkSplit << "\n" << content << "\n"
      << kMarkEnd << "\n";
    f.close();
    return f.good();
}

std::vector<HistoryEntry> loadHistory(const std::string &historyFile)
{
    std::vector<HistoryEntry> entries;
    std::ifstream f(historyFile);
    if (!f.is_open()) return entries;

    enum class Phase { None, Cmd, Content };
    Phase phase = Phase::None;
    HistoryEntry cur;
    std::string line;
    while (std::getline(f, line)) {
        if (line == kMarkCmd) {
            cur = HistoryEntry{};
            phase = Phase::Cmd;
        } else if (line == kMarkSplit) {
            phase = Phase::Content;
        } else if (line == kMarkEnd) {
            phase = Phase::None;
            entries.push_back(cur);
        } else {
            if (phase == Phase::Cmd) {
                if (!cur.cmd.empty()) cur.cmd += "\n";
                cur.cmd += line;
            } else if (phase == Phase::Content) {
                if (!cur.content.empty()) cur.content += "\n";
                cur.content += line;
            }
        }
    }
    return entries;
}

bool loadLast(const std::string &historyFile, HistoryEntry &out)
{
    auto entries = loadHistory(historyFile);
    if (entries.empty()) return false;
    out = entries.back();
    return true;
}

} // namespace explain

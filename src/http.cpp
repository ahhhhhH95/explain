#include "explain/http.h"
#include "explain/util.h"

#include <curl/curl.h>

#include <string>

namespace explain {

namespace {

// libcurl 写回调：把响应体追加到字符串
size_t writeCallback(char *buf, size_t size, size_t nmemb, void *userdata)
{
    std::string *s = static_cast<std::string *>(userdata);
    s->append(buf, size * nmemb);
    return size * nmemb;
}

// 发送 JSON 请求到 API，并检查 HTTP 状态码。
// 成功（curl 无错误且 HTTP 200）返回 true；否则在 err 中给出具体报错。
bool httpPostJson(const Config &cfg, const std::string &postData,
                  std::string &resp, long &httpCode, std::string &err)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        err = "curl 初始化失败";
        return false;
    }

    struct curl_slist *hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    std::string auth = "Authorization: Bearer " + cfg.apiKey;
    hdrs = curl_slist_append(hdrs, auth.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, cfg.apiUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, cfg.timeoutSec);
    // 默认校验 HTTPS 证书（修复原版关闭校验的问题），可通过 ssl_verify 配置关闭
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, cfg.sslVerify ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, cfg.sslVerify ? 2L : 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "explain/1.1.0");

    CURLcode ret = curl_easy_perform(curl);
    httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (ret != CURLE_OK) {
        err = "API 请求失败: " + std::string(curl_easy_strerror(ret));
        return false;
    }
    if (httpCode != 200) {
        err = "API 返回 HTTP " + std::to_string(httpCode);
        if (!resp.empty()) {
            std::string body = resp.size() > 300 ? resp.substr(0, 300) + "..." : resp;
            err += ": " + body;
        }
        return false;
    }
    return true;
}

// 从 API 响应中解析 content 字段；失败时在 err 中给出响应片段
bool parseContent(const std::string &resp, std::string &content, std::string &err)
{
    if (!extractJsonStringValue(resp, "content", content)) {
        err = "无法从 API 响应中解析 content 字段";
        if (!resp.empty()) {
            std::string frag = resp.size() > 200 ? resp.substr(0, 200) + "..." : resp;
            err += "。响应片段: " + frag;
        }
        return false;
    }
    return true;
}

} // namespace

AiResult callAI(const Config &cfg, const std::string &manText, const std::string &cmdline)
{
    AiResult res;
    if (cfg.apiKey.empty()) {
        res.err = "未配置 API Key：请在 ~/.explainrc 设置 api_key，或设置环境变量 EXPLAIN_API_KEY";
        return res;
    }
    if (cfg.apiUrl.empty() || cfg.model.empty()) {
        res.err = "配置文件缺少 api_url 或 model";
        return res;
    }

    std::string prompt =
        "你是专业 Linux 命令助手。请根据下面的 man 手册原文，用中文简洁解释命令【" + cmdline + "】。"
        "输出严格分为三个部分：【命令作用】、【参数详解】、【实用示例】，不要多余废话。\n"
        "man 手册原文：\n" + manText;
    std::string postData =
        "{\"model\":\"" + jsonEscape(cfg.model) +
        "\",\"stream\":false,\"messages\":[{\"role\":\"user\",\"content\":\"" +
        jsonEscape(prompt) + "\"}]}";

    std::string resp;
    long httpCode = 0;
    std::string err;
    if (!httpPostJson(cfg, postData, resp, httpCode, err)) {
        res.err = err;
        return res;
    }
    if (!parseContent(resp, res.content, err)) {
        res.err = err;
        return res;
    }
    res.ok = true;
    return res;
}

AiResult testConnection(const Config &cfg)
{
    AiResult res;
    if (cfg.apiKey.empty()) {
        res.err = "未配置 API Key：请在 ~/.explainrc 设置 api_key，或设置环境变量 EXPLAIN_API_KEY";
        return res;
    }
    if (cfg.apiUrl.empty() || cfg.model.empty()) {
        res.err = "配置文件缺少 api_url 或 model";
        return res;
    }

    // 最小探测请求：让模型简短回复，验证鉴权、网络、模型可用性
    std::string postData =
        "{\"model\":\"" + jsonEscape(cfg.model) +
        "\",\"stream\":false,\"max_tokens\":16,\"messages\":[{\"role\":\"user\",\"content\":\"请只回复两个字：正常\"}]}";

    std::string resp;
    long httpCode = 0;
    std::string err;
    if (!httpPostJson(cfg, postData, resp, httpCode, err)) {
        res.err = err;
        return res;
    }
    if (!parseContent(resp, res.content, err)) {
        res.err = err;
        return res;
    }
    res.ok = true;
    return res;
}

} // namespace explain

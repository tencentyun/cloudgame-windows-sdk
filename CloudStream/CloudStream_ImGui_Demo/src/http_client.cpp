#include "http_client.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "logger.h"

#if defined(_WIN32)
#  define popen _popen
#  define pclose _pclose
#endif

// 通过系统 curl 命令执行 HTTP POST（简单可靠，跨平台）
// 生产环境建议替换为 libcurl 或平台原生网络库
HttpResponse http_post(const std::string& url, const std::string& json_body) {
  HttpResponse resp;

  // 构建 curl 命令
  // -s: 静默模式
  // -w "\n%{http_code}": 在末尾输出 HTTP 状态码
  // -H: 设置 Content-Type
  // -d: POST body
  // --connect-timeout: 连接超时 10 秒
  // --max-time: 总超时 30 秒
  std::string cmd =
      "curl -s -w \"\\n%{http_code}\" "
      "--connect-timeout 10 --max-time 30 "
      "-H \"Content-Type: application/json\" "
      "-d '";

  // 转义单引号（将 ' 替换为 '\''）
  for (char c : json_body) {
    if (c == '\'') {
      cmd += "'\\''";
    } else {
      cmd += c;
    }
  }
  cmd += "' '";
  cmd += url;
  cmd += "' 2>&1";

  LOG_INFO("HttpClient", "POST %s", url.c_str());
  LOG_DEBUG("HttpClient", "Body: %s", json_body.c_str());

  // 执行 curl
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    resp.error = "Failed to execute curl command";
    LOG_ERROR("HttpClient", "%s", resp.error.c_str());
    return resp;
  }

  // 读取输出
  std::string output;
  std::array<char, 4096> buf;
  while (fgets(buf.data(), (int)buf.size(), pipe) != nullptr) {
    output += buf.data();
  }

  int exit_code = pclose(pipe);
  if (exit_code != 0) {
    resp.error = "curl failed with exit code " + std::to_string(exit_code) + ": " + output;
    LOG_ERROR("HttpClient", "%s", resp.error.c_str());
    return resp;
  }

  // 解析输出：最后一行是 HTTP 状态码
  size_t last_newline = output.rfind('\n');
  if (last_newline != std::string::npos && last_newline > 0) {
    // 找到倒数第二个换行符之前的内容是 body
    size_t second_last = output.rfind('\n', last_newline - 1);
    if (second_last != std::string::npos) {
      resp.body = output.substr(0, second_last);
      std::string status_str = output.substr(last_newline + 1);
      // 去除尾部空白
      while (!status_str.empty() &&
             (status_str.back() == '\n' || status_str.back() == '\r' || status_str.back() == ' ')) {
        status_str.pop_back();
      }
      resp.status_code = std::atoi(status_str.c_str());
    } else {
      // body 可能为空，最后一行是状态码
      std::string status_str = output.substr(last_newline + 1);
      while (!status_str.empty() &&
             (status_str.back() == '\n' || status_str.back() == '\r' || status_str.back() == ' ')) {
        status_str.pop_back();
      }
      resp.body = output.substr(0, last_newline);
      resp.status_code = std::atoi(status_str.c_str());
    }
  } else {
    // 无换行符 —— 可能全是状态码或全是 body
    resp.body = output;
    resp.status_code = 0;
  }

  LOG_INFO("HttpClient", "Response status: %d, body length: %zu", resp.status_code, resp.body.size());
  LOG_DEBUG("HttpClient", "Response body: %s", resp.body.c_str());

  return resp;
}

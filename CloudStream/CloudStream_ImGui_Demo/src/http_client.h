#pragma once

// http_client.h - 简易 HTTP POST 客户端（跨平台，无额外依赖）
// 用于调用业务后台 API 获取 Token 和 AccessInfo

#include <string>

struct HttpResponse {
  int status_code = 0;
  std::string body;
  std::string error;

  bool ok() const { return status_code >= 200 && status_code < 300; }
};

// 同步 HTTP POST 请求（JSON body）
// url: 完整 URL (https://host/path)
// json_body: JSON 字符串
// 注意：此实现使用系统 curl 命令，兼容 macOS/Linux/Windows (Git Bash)
HttpResponse http_post(const std::string& url, const std::string& json_body);

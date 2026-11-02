#pragma once

/**
 * @brief 本地构建的扩展点（内部调试需要）
 *
 */
class LocalBuildHooks {
 public:
  virtual ~LocalBuildHooks() {}

  /// TcrSdk 客户端初始化成功之后调用
  virtual void onClientInitialized() {}
};

/// 返回本地构建提供的 hook，外部构建返回 nullptr
LocalBuildHooks* get_local_build_hooks();

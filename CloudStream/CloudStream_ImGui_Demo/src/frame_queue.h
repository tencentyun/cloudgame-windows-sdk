#pragma once

// frame_queue.h - 线程安全的单帧缓冲
// 用于 TcrSDK 解码线程和主渲染线程之间的帧传递
// 策略：最新帧覆盖旧帧（丢帧而非延迟）

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

#include "tcr_c_api.h"

// 持有一个 TcrVideoFrame 的引用，RAII 自动释放
struct VideoFrame {
  TcrVideoFrameHandle handle = nullptr;

  // I420 数据指针（不拥有数据，生命周期由 handle 管理）
  const uint8_t* data_y = nullptr;
  const uint8_t* data_u = nullptr;
  const uint8_t* data_v = nullptr;
  int stride_y = 0;
  int stride_u = 0;
  int stride_v = 0;
  int width = 0;
  int height = 0;
  int64_t timestamp_us = 0;

  VideoFrame() = default;

  // 从 TcrSDK 帧数据构造（调用者需先 add_ref）
  VideoFrame(TcrVideoFrameHandle h, const uint8_t* y, const uint8_t* u, const uint8_t* v, int sy, int su, int sv, int w,
             int h_val, int64_t ts)
      : handle(h),
        data_y(y),
        data_u(u),
        data_v(v),
        stride_y(sy),
        stride_u(su),
        stride_v(sv),
        width(w),
        height(h_val),
        timestamp_us(ts) {}

  ~VideoFrame() {
    if (handle) {
      tcr_video_frame_release(handle);
      handle = nullptr;
    }
  }

  // 不可复制
  VideoFrame(const VideoFrame&) = delete;
  VideoFrame& operator=(const VideoFrame&) = delete;

  // 可移动
  VideoFrame(VideoFrame&& other) noexcept
      : handle(other.handle),
        data_y(other.data_y),
        data_u(other.data_u),
        data_v(other.data_v),
        stride_y(other.stride_y),
        stride_u(other.stride_u),
        stride_v(other.stride_v),
        width(other.width),
        height(other.height),
        timestamp_us(other.timestamp_us) {
    other.handle = nullptr;
  }

  VideoFrame& operator=(VideoFrame&& other) noexcept {
    if (this != &other) {
      if (handle) tcr_video_frame_release(handle);
      handle = other.handle;
      data_y = other.data_y;
      data_u = other.data_u;
      data_v = other.data_v;
      stride_y = other.stride_y;
      stride_u = other.stride_u;
      stride_v = other.stride_v;
      width = other.width;
      height = other.height;
      timestamp_us = other.timestamp_us;
      other.handle = nullptr;
    }
    return *this;
  }

  bool valid() const { return handle != nullptr && width > 0 && height > 0; }
};

// 线程安全的单帧缓冲：最新帧覆盖策略
class FrameQueue {
 public:
  // 由解码线程调用，推入新帧（旧帧自动释放）
  void push(VideoFrame&& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frame = std::move(frame);
    m_has_new = true;
  }

  // 由主线程调用，取出最新帧
  // 返回 true 表示有新帧，false 表示无新帧
  bool pop(VideoFrame& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_has_new) return false;
    out = std::move(m_frame);
    m_has_new = false;
    return true;
  }

  // 清空缓冲
  void clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frame = VideoFrame();
    m_has_new = false;
  }

 private:
  std::mutex m_mutex;
  VideoFrame m_frame;
  bool m_has_new = false;
};

// 多实例帧缓存：每个 instance_id 对应一个最新帧
// 用于多流场景下，解码线程按 instance_id 路由帧到主线程
class MultiFrameCache {
 public:
  // 由解码线程调用：缓存指定实例的最新帧
  void push(const std::string& instance_id, VideoFrame&& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frames[instance_id] = std::move(frame);
  }

  // 由主线程调用：原子地取出所有缓存帧
  // 返回后内部缓存为空
  std::map<std::string, VideoFrame> swap_all() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, VideoFrame> result;
    std::swap(result, m_frames);
    return result;
  }

  // 清空缓存
  void clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frames.clear();
  }

 private:
  std::mutex m_mutex;
  std::map<std::string, VideoFrame> m_frames;
};

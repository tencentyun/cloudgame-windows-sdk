#ifndef TCRDEMO_VIDEO_RENDERER_H_
#define TCRDEMO_VIDEO_RENDERER_H_
#define NOMINMAX
#include <Windows.h>
#include "video_frame_observer.h"

// A little helper class to make sure we always to proper locking and
// unlocking when working with VideoRenderer buffers.
template <typename T>
class AutoLock {
public:
    explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
    ~AutoLock() { obj_->Unlock(); }

protected:
    T* obj_;
};

class VideoRenderer : public tcrsdk::VideoFrameObserver {
public:
    VideoRenderer(HWND wnd,
        int width,
        int height);
    virtual ~VideoRenderer();

    void Lock() { ::EnterCriticalSection(&buffer_lock_); }

    void Unlock() { ::LeaveCriticalSection(&buffer_lock_); }

    // VideoSinkInterface implementation
    void OnFrame(const tcrsdk::VideoFrame& frame) override;

    const BITMAPINFO& bmi() const { return bmi_; }
    const uint8_t* image() const { return image_.get(); }

    void I420ToARGB(const uint8_t* src_y,
        int src_stride_y,
        const uint8_t* src_u,
        int src_stride_u,
        const uint8_t* src_v,
        int src_stride_v,
        uint8_t* dst_argb,
        int dst_stride_argb,
        int width,
        int height);

protected:
    void SetSize(int width, int height);

    enum {
        SET_SIZE,
        RENDER_FRAME,
    };

    HWND wnd_;
    BITMAPINFO bmi_;
    std::unique_ptr<uint8_t[]> image_;
    CRITICAL_SECTION buffer_lock_;
};

#endif


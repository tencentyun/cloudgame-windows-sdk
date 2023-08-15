#ifndef TCRDEMO_VIDEO_RENDERER_H_
#define TCRDEMO_VIDEO_RENDERER_H_

#include <Windows.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW\glfw3.h>
#include <mutex>
#include "video_frame_observer.h"

class VideoRenderer : public tcrsdk::VideoFrameObserver {
public:
    VideoRenderer(HWND wnd, int32_t viewport_width, int32_t viewport_height);
    virtual ~VideoRenderer();

    // VideoSinkInterface implementation
    void OnFrame(const tcrsdk::VideoFrame& frame) override;
    void OnSurfaceChanged(int32_t width, int32_t height);

private:
    void Init();
    bool InitContext();
    void DestroyContext();
    void InitShaders();
    void InitTexture();
    void CalcRenderPos();
    void RenderFrame(const tcrsdk::VideoFrame& video_frame);

private:
    uint32_t window_width_{};
    uint32_t window_height_{};
    HWND hwnd_{};
    HDC hdc_{};
    HGLRC hrc_{};

    GLuint texture_y_;
    GLuint texture_u_;
    GLuint texture_v_;
    GLuint uniform_y_;
    GLuint uniform_u_;
    GLuint uniform_v_;
    GLuint shader_program_;

    uint32_t render_width_{};
    uint32_t render_height_{}; 

    volatile uint32_t viewport_width_{};
    volatile uint32_t viewport_height_{};

    tcrsdk::VideoFrame video_frame_;

    std::mutex frame_mutex_;
    volatile bool is_frame_active_ = false;
    volatile bool is_inited_ = false;
    volatile bool is_rendering_ = true;

    static const char* const TAG;
};

#endif

